/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/PassTimer.hpp"
#include "Compiler/CISACodeGen/TimeStatsCounter.h"
#include "Compiler/CodeGenPublic.h"
#include "common/debug/Dump.hpp"
#include "common/shaderOverride.hpp"
#include "common/Stats.hpp"
#include "common/IntrinsicAnnotator.hpp"
#include "common/LLVMUtils.h"
#include "common/SerializePrintMetaDataPass.h"
#include "llvmWrapper/ADT/StringRef.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace IGC::Debug;
using namespace llvm;

bool getPassToggles(std::bitset<1024> &toggles) {
  const char *passToggles = IGC_GET_REGKEYSTRING(DisablePassToggles);
  if (passToggles != nullptr && strlen(passToggles) > 0) {
    std::string szBin;
    std::string szHexLL;
    unsigned int len = 0;
    unsigned long long x = 0;
    std::string szHex = passToggles;
    for (size_t i = 0; i < szHex.size(); i += 16) {
      szHexLL = szHex.substr(i, 16);
      len = szHexLL.size() * 4;
      x = std::stoull(szHexLL, nullptr, 16);
      szBin += std::bitset<64>(x).to_string().substr(64 - len, len);
    }

    toggles = std::bitset<1024>(szBin);
    return true;
  }

  return false;
}

llvm::Pass *createFlushPass(llvm::Pass *pass, Dump &dump) {
  // Choose an appropriate pass to preserve the original order of pass execution in the pass manager
  switch (pass->getPassKind()) {
  case PT_Function:
    return new FunctionFlushDumpPass(dump);
  case PT_Module:
    return new ModuleFlushDumpPass(dump);
  case PT_Region:
  case PT_Loop:
  case PT_CallGraphSCC:
    // flushing for analysis passes is not required
    break;
  case PT_PassManager:
  default:
    // internal pass managers are not considered in the context
    break;
  }
  return nullptr;
}

/*
ShaderPassDisable
The syntax is as follows:
ShaderPassDisable="TOKEN1;TOKEN2;..."

TOKEN:
* Pass number                           PASS_NUMBER                                 example: 14
* Range                                 PASS_NUMBER_START-PASS_NUMBER_STOP          example: 10-50
* Open range                            PASS_NUMBER_START-                          example: 60-
* Pass name                             PASS_NAME                                   example: BreakConstantExprPass
* Specific pass occurrence              PASS_NAME:occurrence                        example: BreakConstantExprPass:0
* Specific pass occurrences range       PASS_NAME:OCCURRANCE_START-OCCURRANCE_STOP  example: BreakConstantExprPass:2-4
* Specific pass occurrences open range  PASS_NAME:OCCURRANCE_START-                 example: BreakConstantExprPass:3-

Example:
IGC_ShaderPassDisable="9;17-19;239-;Error Check;ResolveOCLAtomics:2;Dead Code Elimination:3-5;BreakConstantExprPass:7-"

You want to skip pass ID 9
You want to skip passes from 17 to 19
You want to skip passes from 239 to end
You want to skip all occurrence of Error Check
You want to skip pass ResolveOCLAtomics occurrence 2
You want to skip pass Dead Code Elimination from occurrence 3 to occurrence 5
You want to skip pass BreakConstantExprPass from occurrence 7 to end

Pass number    Pass name                                         Pass occurrence     skippedBy
9              LowerInvokeSIMD                                   1                   PassNumber
17             CorrectlyRoundedDivSqrt                           1                   Range
18             CodeAssumption                                    1                   Range
19             JointMatrixFuncsResolutionPass                    1                   Range
32             Error Check                                       1                   PassName
65             ResolveOCLAtomics                                 2                   SpecificPassOccurrence
75             Dead Code Elimination                             3                   SpecificPassOccurrenceRange
77             Error Check                                       2                   PassName
121            Dead Code Elimination                             4                   SpecificPassOccurrenceRange
143            Dead Code Elimination                             5                   SpecificPassOccurrenceRange
145            BreakConstantExprPass                             7                   SpecificPassOccurrenceOpenRange
181            BreakConstantExprPass                             8                   SpecificPassOccurrenceOpenRange
183            BreakConstantExprPass                             9                   SpecificPassOccurrenceOpenRange
189            BreakConstantExprPass                             10                  SpecificPassOccurrenceOpenRange
214            BreakConstantExprPass                             11                  SpecificPassOccurrenceOpenRange
221            BreakConstantExprPass                             12                  SpecificPassOccurrenceOpenRange
239            Layout                                            1                   OpenRange
240            TimeStatsCounter Start/Stop                       6                   OpenRange
241            EmitPass                                          1                   OpenRange
242            EmitPass                                          2                   OpenRange
243            EmitPass                                          3                   OpenRange
244            DebugInfoPass                                     1                   OpenRange
*/
enum TokenSkipCase {
  PassNumber,
  Range,
  OpenRange,
  PassName,
  SpecificPassOccurrence,
  SpecificPassOccurrenceRange,
  SpecificPassOccurrenceOpenRange,
  WrongFormat
};

struct SkipCommand {
  std::string name;
  unsigned int start, end;
  TokenSkipCase skippedBy;

  SkipCommand(const std::string &_name, unsigned _start, unsigned _end, TokenSkipCase _skippedBy)
      : name(_name), start(_start), end(_end), skippedBy(_skippedBy) {}

  SkipCommand(const std::string &_name) : name(_name) {
    start = std::numeric_limits<unsigned int>::max();
    end = std::numeric_limits<unsigned int>::max();
    skippedBy = TokenSkipCase::PassName;
  }

  SkipCommand(unsigned _start, unsigned _end) : start(_start), end(_end) {
    name = "";
    skippedBy = TokenSkipCase::Range;
  }

  SkipCommand(unsigned _start) : start(_start), end(_start) {
    name = "";
    skippedBy = TokenSkipCase::PassNumber;
  }
};

struct PassDisableConfig {
  std::unordered_map<std::string, unsigned int> passesOccuranceCount;
  std::vector<SkipCommand> skipCommands;
  unsigned int passCount = 0;

  PassDisableConfig();
};

static bool tryParsePassNumber(PassDisableConfig &pdc, const std::string &Token) {
  // remove spaces to check if other chars is only digit
  std::string tempTokenWithoutSpaces = Token;
  tempTokenWithoutSpaces.erase(remove(tempTokenWithoutSpaces.begin(), tempTokenWithoutSpaces.end(), ' '),
                               tempTokenWithoutSpaces.end());
  if (std::all_of(tempTokenWithoutSpaces.cbegin(), tempTokenWithoutSpaces.cend(), ::isdigit)) {
    std::cerr << "You want to skip pass ID " << Token << std::endl;
    unsigned int passNumber = std::stoi(Token);
    pdc.skipCommands.emplace_back(passNumber);
    // move to next Token
    return true;
  }
  // continue to parse this Token
  return false;
}

static bool checkSkipCommandBounds(unsigned int from, unsigned int to) {
  if (from > to) {
    std::cerr << "You want to skip passes from " << from << " to " << to
              << " but start of skipping is bigger than end of skipping" << std::endl;
    return false;
  }
  return true;
}

static SkipCommand *getFirstCommand(PassDisableConfig &pdc, TokenSkipCase commandCase,
                                    const std::string &passName = "") {
  auto it = find_if(pdc.skipCommands.begin(), pdc.skipCommands.end(), [&passName, &commandCase](SkipCommand &sc) {
    return sc.skippedBy == commandCase && (passName.empty() || sc.name == passName);
  });

  return it == pdc.skipCommands.end() ? nullptr : &(*it);
}

static bool tryParsePassRange(PassDisableConfig &pdc, const std::string &Token) {
  // remove '-' and spaces to check if other chars is only digit
  std::string tempTokenWithoutDashAndSpaces = Token;
  tempTokenWithoutDashAndSpaces.erase(
      remove(tempTokenWithoutDashAndSpaces.begin(), tempTokenWithoutDashAndSpaces.end(), '-'),
      tempTokenWithoutDashAndSpaces.end());
  tempTokenWithoutDashAndSpaces.erase(
      remove(tempTokenWithoutDashAndSpaces.begin(), tempTokenWithoutDashAndSpaces.end(), ' '),
      tempTokenWithoutDashAndSpaces.end());
  if (!std::all_of(tempTokenWithoutDashAndSpaces.cbegin(), tempTokenWithoutDashAndSpaces.cend(), ::isdigit)) {
    // continue to parse this Token
    return false;
  }

  std::istringstream tokenAsStringStream(Token);
  std::string subToken;
  std::vector<std::string> vectorOfSubTokens;

  while (std::getline(tokenAsStringStream, subToken, '-')) {
    vectorOfSubTokens.push_back(subToken);
  }

  switch (vectorOfSubTokens.size()) {
  // Parse cases for Token: Range
  case 2: {
    unsigned int skippingFrom = std::stoi(vectorOfSubTokens[0]);
    unsigned int skippingTo = std::stoi(vectorOfSubTokens[1]);
    if (checkSkipCommandBounds(skippingFrom, skippingTo)) {
      std::cerr << "You want to skip passes from " << skippingFrom << " to " << skippingTo << std::endl;
      pdc.skipCommands.emplace_back(skippingFrom, skippingTo);
    }
    break;
  }
  // Parse cases for Token: Open range
  case 1: {
    unsigned int skippingFrom = std::stoi(vectorOfSubTokens[0]);
    std::cerr << "You want to skip passes from " << skippingFrom << " to end" << std::endl;

    if (SkipCommand *sc = getFirstCommand(pdc, TokenSkipCase::OpenRange)) {
      sc->start = std::min(sc->start, skippingFrom);
    } else {
      pdc.skipCommands.emplace_back("", skippingFrom, std::numeric_limits<unsigned int>::max(),
                                    TokenSkipCase::OpenRange);
    }
    break;
  }
  // Wrong Token format
  default: {
    std::cerr << "Wrong format TOKEN, TOKEN should have one '-'. Your Token: " << Token << std::endl;
  }
  }
  // move to next Token
  return true;
}

static bool tryParsePassNameSpecificOccurrence(PassDisableConfig &pdc, const std::string &nameOfPassToSkip,
                                               const std::string &occurrenceOfPass) {
  // remove spaces to check if other chars is only digit
  std::string tempOccurrenceOfPassWithoutSpaces = occurrenceOfPass;
  tempOccurrenceOfPassWithoutSpaces.erase(
      remove(tempOccurrenceOfPassWithoutSpaces.begin(), tempOccurrenceOfPassWithoutSpaces.end(), ' '),
      tempOccurrenceOfPassWithoutSpaces.end());
  if (!std::all_of(tempOccurrenceOfPassWithoutSpaces.cbegin(), tempOccurrenceOfPassWithoutSpaces.cend(), ::isdigit)) {
    // continue to parse this Token
    return false;
  }
  std::cerr << "You want to skip pass " << nameOfPassToSkip << " occurrence " << occurrenceOfPass << std::endl;
  unsigned int passOccurrence = std::stoi(occurrenceOfPass);
  pdc.skipCommands.emplace_back(nameOfPassToSkip, passOccurrence, passOccurrence,
                                TokenSkipCase::SpecificPassOccurrence);
  // move to next Token
  return true;
}

static bool tryParsePassNameOccurrenceRange(PassDisableConfig &pdc, const std::string &Token,
                                            const std::string &nameOfPassToSkip, const std::string &occurrenceOfPass) {
  // remove '-' to check if other chars is only digit
  std::string tempOccurrenceOfPassWithoutDashAndSpaces = occurrenceOfPass;
  tempOccurrenceOfPassWithoutDashAndSpaces.erase(
      remove(tempOccurrenceOfPassWithoutDashAndSpaces.begin(), tempOccurrenceOfPassWithoutDashAndSpaces.end(), '-'),
      tempOccurrenceOfPassWithoutDashAndSpaces.end());
  tempOccurrenceOfPassWithoutDashAndSpaces.erase(
      remove(tempOccurrenceOfPassWithoutDashAndSpaces.begin(), tempOccurrenceOfPassWithoutDashAndSpaces.end(), ' '),
      tempOccurrenceOfPassWithoutDashAndSpaces.end());
  if (!std::all_of(tempOccurrenceOfPassWithoutDashAndSpaces.cbegin(), tempOccurrenceOfPassWithoutDashAndSpaces.cend(),
                   ::isdigit)) {
    std::cerr << "Wrong format TOKEN " << Token << " should have digit or '-' after ':'" << std::endl;
    // move to next Token anyway
    return true;
  }

  std::istringstream occurrenceOfPassAsStringStream(occurrenceOfPass);
  std::string occurrenceOfPassSubToken;
  std::vector<std::string> occurrenceOfPassVectorOfSubTokens;

  while (std::getline(occurrenceOfPassAsStringStream, occurrenceOfPassSubToken, '-')) {
    occurrenceOfPassVectorOfSubTokens.push_back(occurrenceOfPassSubToken);
  }

  switch (occurrenceOfPassVectorOfSubTokens.size()) {
  // Parse cases for Token: Specific pass occurrences range
  case 2: {
    unsigned int skippingFrom = std::stoi(occurrenceOfPassVectorOfSubTokens[0]);
    unsigned int skippingTo = std::stoi(occurrenceOfPassVectorOfSubTokens[1]);
    if (checkSkipCommandBounds(skippingFrom, skippingTo)) {
      std::cerr << "You want to skip pass " << nameOfPassToSkip << " from occurrence " << skippingFrom
                << " to occurrence " << skippingTo << std::endl;
      pdc.skipCommands.emplace_back(nameOfPassToSkip, skippingFrom, skippingTo,
                                    TokenSkipCase::SpecificPassOccurrenceRange);
    }
    break;
  }
  // Parse cases for Token: Specific pass occurrences open range
  case 1: {
    unsigned int skippingFrom = std::stoi(occurrenceOfPassVectorOfSubTokens[0]);
    std::cerr << "You want to skip pass " << nameOfPassToSkip << " from occurrence " << skippingFrom << " to end"
              << std::endl;

    if (SkipCommand *sc = getFirstCommand(pdc, TokenSkipCase::SpecificPassOccurrenceOpenRange, nameOfPassToSkip)) {
      sc->start = std::min(sc->start, skippingFrom);
    } else {
      pdc.skipCommands.emplace_back(nameOfPassToSkip, skippingFrom, std::numeric_limits<unsigned int>::max(),
                                    TokenSkipCase::SpecificPassOccurrenceOpenRange);
    }
    break;
  }
  // Wrong Token format
  default: {
    std::cerr << "Wrong format TOKEN, TOKEN should have one '-'. Your Token: " << Token << std::endl;
  }
  }
  // move to next Token
  return true;
}

static bool tryParsePassNames(PassDisableConfig &pdc, const std::string &Token) {
  std::istringstream tokenAsStringStream(Token);
  std::string subToken;
  std::vector<std::string> vectorOfSubTokens;

  while (std::getline(tokenAsStringStream, subToken, ':')) {
    vectorOfSubTokens.push_back(subToken);
  }

  switch (vectorOfSubTokens.size()) {
  // Parse cases for Token: Specific pass occurrence AND
  // Specific pass occurrences range AND
  // Specific pass occurrences open range
  case 2: {
    std::string nameOfPassToSkip = vectorOfSubTokens[0];
    std::string occurrenceOfPass = vectorOfSubTokens[1];

    // Parse cases for Token: Specific pass occurrence
    if (tryParsePassNameSpecificOccurrence(pdc, nameOfPassToSkip, occurrenceOfPass))
      return true; // move to next Token

    // Parse cases for Token: Specific pass occurrences range AND Specific pass occurrences open range
    if (tryParsePassNameOccurrenceRange(pdc, Token, nameOfPassToSkip, occurrenceOfPass))
      return true; // move to next Token

    return true; // move to next Token anyway
  }
  // Parse cases for Token: Pass name: PASS_NAME
  case 1: {
    const std::string &nameOfPassToSkip = vectorOfSubTokens[0];
    std::cerr << "You want to skip all occurrence of " << nameOfPassToSkip << std::endl;
    pdc.skipCommands.emplace_back(nameOfPassToSkip);
    // move to next Token
    return true;
  }
  // Wrong Token format
  default: {
    std::cerr << "Wrong format TOKEN, TOKEN should have one ':'. Your Token: " << Token << std::endl;
    // move to next Token anyway
    return true;
  }
  }
}

static void parseShaderPassDisableFlag(PassDisableConfig &pdc) {
  unsigned int passToSkip = IGC_GET_FLAG_VALUE(ShaderPassDisable);

  // Using std::cerr caused crashes in SYCL environment so we use fprintf instead as a work around.
  std::ostringstream debugOutputStream;

  // When disabling a single pass (eg. IGC_ShaderPassDisable=94) the number
  // will be parsed as ASCII char by IGC_GET_REGKEYSTRING.
  // In this case IGC_GET_FLAG_VALUE will return the char value.
  // If ShaderPassDisable was set to a string, IGC_GET_FLAG_VALUE will return an address.
  // We compare to 0xFFFF to naively check for number/string.
  if (passToSkip < 0xFFFF) {
    debugOutputStream << "Your input to ShaderPassDisable: " << passToSkip << '\n';
    debugOutputStream << "You want to skip pass ID " << passToSkip << '\n';

    pdc.skipCommands.emplace_back(passToSkip);
  }
  // Treat it as a string otherwise
  else {
    std::string passesToSkip = std::string(IGC_GET_REGKEYSTRING(ShaderPassDisable));

    debugOutputStream << "Your input to ShaderPassDisable: " << passesToSkip << '\n';

    std::string Token;
    std::istringstream passesToSkipAsStringStream(passesToSkip);
    // Split to Tokens
    while (std::getline(passesToSkipAsStringStream, Token, ';')) {
      // Parse case for Token: Pass number
      if (tryParsePassNumber(pdc, Token))
        continue;

      // Parse cases for Token: Range AND Open range
      if (tryParsePassRange(pdc, Token))
        continue;

      // Parse cases for Token: Pass name AND Specific pass occurrence AND
      // Specific pass occurrences range AND
      // Specific pass occurrences open range
      if (tryParsePassNames(pdc, Token))
        continue;
    }
  }

  fprintf(stderr, "%s", debugOutputStream.str().c_str());
}

PassDisableConfig::PassDisableConfig() { parseShaderPassDisableFlag(*this); }

const SkipCommand *findApplicableSkipCommand(const PassDisableConfig &pdc, const std::string &currentPassName) {
  for (auto sc = pdc.skipCommands.cbegin(); sc != pdc.skipCommands.cend(); sc++) {
    if (sc->name.empty() && pdc.passCount >= sc->start && pdc.passCount <= sc->end) {
      // Pass number OR Range OR Open range
      return &(*sc);
    }

    if (sc->name != currentPassName)
      continue;

    auto currentPass = pdc.passesOccuranceCount.find(currentPassName);
    if (currentPass == pdc.passesOccuranceCount.end()) {
      continue;
    } else {
      if (sc->name == currentPassName && currentPass->second >= sc->start && currentPass->second <= sc->end) {
        // Specific pass occurrence OR Specific pass occurrences range OR Specific pass occurrences open range
        return &(*sc);
      }
      if (sc->name == currentPassName && sc->start == std::numeric_limits<unsigned int>::max() &&
          sc->end == std::numeric_limits<unsigned int>::max()) {
        // Pass name
        return &(*sc);
      }
    }
  }
  return nullptr;
}

void displaySkippedPass(const PassDisableConfig &pdc, const std::string &currentPassName,
                        const SkipCommand *skippedByCommand) {
  // Using std::cerr caused crashes in SYCL environment so we use fprintf instead as a work around.
  std::ostringstream debugOutputStream;

  static bool displayHeader = true;
  std::string skippedByString = "Invalid skip pass case";
  TokenSkipCase skippedBy = WrongFormat;

  if (skippedByCommand)
    skippedBy = skippedByCommand->skippedBy;

  switch (skippedBy) {
#define CMD_KIND_CASE(x)                                                                                               \
  case x:                                                                                                              \
    skippedByString = #x;                                                                                              \
    break;
    CMD_KIND_CASE(Range)
    CMD_KIND_CASE(OpenRange)
    CMD_KIND_CASE(PassNumber)
    CMD_KIND_CASE(PassName)
    CMD_KIND_CASE(SpecificPassOccurrence)
    CMD_KIND_CASE(SpecificPassOccurrenceRange)
    CMD_KIND_CASE(SpecificPassOccurrenceOpenRange)
    CMD_KIND_CASE(WrongFormat)
#undef CMD_KIND_CASE
  }

  if (displayHeader) {
    debugOutputStream << std::left << '\n'
                      << setw(15) << "Pass number" << setw(50) << "Pass name" << setw(20) << "Pass occurrence"
                      << setw(75) << "skippedBy" << '\n';
    displayHeader = false;
  }

  auto singlePass = pdc.passesOccuranceCount.find(currentPassName);

  if (singlePass != pdc.passesOccuranceCount.end()) {
    debugOutputStream << std::left << setw(15) << pdc.passCount << setw(50) << currentPassName << setw(20)
                      << singlePass->second << setw(75) << skippedByString << '\n';
  } else {
    debugOutputStream << std::left << "You are trying to skip a pass that is not available" << '\n';
  }

  fprintf(stderr, "%s", debugOutputStream.str().c_str());
}

/*
ShaderDisplayAllPassesNames

Example:
IGC_ShaderDisplayAllPassesNames=1

Pass number Pass name                                         Pass occurrence
0           CheckInstrTypes                                   1
1           Types Legalization Pass                           1
2           Target Library Information                        1
3           Dead Code Elimination                             1
...
...
...
225         Layout                                            1
226         TimeStatsCounter Start/Stop                       6
227         EmitPass                                          1
228         EmitPass                                          2
229         EmitPass                                          3
230         DebugInfoPass                                     1
Build succeeded.
*/

void displayAllPasses(const Pass *P) {
  // Using std::cerr caused crashes in SYCL environment so we use fprintf instead as a work around.
  std::ostringstream debugOutputStream;

  static unsigned int countPass = 0;
  const std::string currentPassName = P->getPassName().str();
  static std::unordered_map<std::string, int> passesOccuranceCountToDisplay;

  auto result = passesOccuranceCountToDisplay.try_emplace(currentPassName, 1);
  if (!result.second) {
    result.first->second += 1;
  }

  if (!countPass) {
    debugOutputStream << std::left << '\n'
                      << setw(12) << "Pass number" << setw(50) << "Pass name" << setw(20) << "Pass occurrence" << '\n';
  }

  debugOutputStream << std::left << setw(12) << countPass << setw(50) << currentPassName << setw(20)
                    << passesOccuranceCountToDisplay[currentPassName] << '\n';

  countPass++;

  fprintf(stderr, "%s", debugOutputStream.str().c_str());
}

void IGCPassManager::add(Pass *P) {
  if (IGC_IS_FLAG_ENABLED(ShaderDumpTranslationOnly))
    return;

  // check only once
  static bool checkedToggles = false;
  static bool hasToggles = false;
  static std::bitset<1024> toggles;
  if (!checkedToggles) {
    checkedToggles = true;
    hasToggles = getPassToggles(toggles);
  }
  if (hasToggles && m_pContext->m_numPasses < 1024 && toggles[m_pContext->m_numPasses]) {
    errs() << "Skipping pass: '" << P->getPassName() << "\n";
    m_pContext->m_numPasses++;
    return;
  }

  if (IGC_IS_FLAG_ENABLED(ShaderDisableOptPassesAfter) &&
      m_pContext->m_numPasses > IGC_GET_FLAG_VALUE(ShaderDisableOptPassesAfter) && m_name == "OPT") {
    errs() << "Skipping optimization pass: '" << P->getPassName()
           << "' (threshold: " << IGC_GET_FLAG_VALUE(ShaderDisableOptPassesAfter) << ").\n";
    return;
  }

  if (IGC_IS_FLAG_ENABLED(ShaderPassDisable)) {
    // Parse Flag Input once in PassDisableConfig constructor
    static PassDisableConfig pdc;
    const std::string currentPassName = P->getPassName().str();

    // Fill the passesOccuranceCount
    auto result = pdc.passesOccuranceCount.try_emplace(currentPassName, 1);
    if (!result.second) {
      result.first->second += 1;
    }

    // Check if current pass should by skipped
    const SkipCommand *skippedByCommand = findApplicableSkipCommand(pdc, currentPassName);
    if (skippedByCommand) {
      displaySkippedPass(pdc, currentPassName, skippedByCommand);
      m_pContext->m_numPasses++;
      pdc.passCount++;
      return;
    }
    pdc.passCount++;
  }

  if (IGC_IS_FLAG_ENABLED(ShaderDisplayAllPassesNames) && !IGC_IS_FLAG_ENABLED(ShaderPassDisable)) {
    displayAllPasses(P);
  }

  // Skip adding a printer pass for analysis passes.
  std::string pname(P->getPassName().str());
  auto isAnalysisPass = [&]() {
    const PassInfo *PI = Pass::lookupPassInfo(P->getPassID());
    if (PI && PI->isAnalysis())
      return true;
    else
      return false; // unknown or not an analysis pass
  }();

  if (!isAnalysisPass && isPrintBefore(P)) {
    addPrintPass(P, true);
  }

  if (IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsPerPass, TIME_STATS_PER_PASS)) {
    PassManager::add(createTimeStatsIGCPass(m_pContext, m_name + '_' + pname, STATS_COUNTER_START));
  }

  PassManager::add(P);

  if (IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsPerPass, TIME_STATS_PER_PASS)) {
    PassManager::add(createTimeStatsIGCPass(m_pContext, m_name + '_' + pname, STATS_COUNTER_END));
  }

  if (!isAnalysisPass && isPrintAfter(P)) {
    addPrintPass(P, false);
  }
}

// should be an std::string_view, but StringRef doesn't implement implicit conversion unless __cplusplus > 201402L
template <typename StringT> inline static std::string cleanPassName(const StringT &passName) {
  // Remove non-alphanumeric characters from pass name
  std::string newName;
  std::copy_if(passName.begin(), passName.end(), std::back_inserter(newName),
               [](unsigned char c) { return std::isalnum(c); });
  return newName;
}

// List: a comma/semicolon-separated list of pass names.
//    N: a pass name
// return true if N is in List.
bool IGCPassManager::isInList(const StringRef &N, const StringRef &List) const {
  StringRef Separators(",;");
  size_t startPos = 0;
  while (startPos != StringRef::npos) {
    size_t endPos = List.find_first_of(Separators, startPos);
    size_t len = (endPos != StringRef::npos ? endPos - startPos : endPos);
    StringRef Name = List.substr(startPos, len);
    if (IGCLLVM::equals_insensitive(cleanPassName(Name), cleanPassName(N))) {
      return true;
    }
    startPos = (endPos != StringRef::npos ? endPos + 1 : StringRef::npos);
  }
  return false;
}

bool IGCPassManager::isPrintBefore(Pass *P) {
  if (IGC_IS_FLAG_ENABLED(PrintBefore)) {
    // PrintBefore=N0,N1,N2  : comma-separate list of pass names
    //                         or pass command args registered in passInfo.
    StringRef passNameList(IGC_GET_REGKEYSTRING(PrintBefore));
    StringRef PN = P->getPassName();
    if (IGCLLVM::equals_insensitive(passNameList, "all") || isInList(PN, passNameList))
      return true;

    // further check passInfo
    if (const PassInfo *PI = Pass::lookupPassInfo(P->getPassID())) {
      return isInList(PI->getPassArgument(), passNameList);
    }
  }
  return false;
}

bool IGCPassManager::isPrintAfter(Pass *P) {
  if (IGC_IS_FLAG_ENABLED(ShaderDumpEnableAll)) {
    return true;
  }
  if (IGC_IS_FLAG_ENABLED(PrintAfter)) {
    // PrintAfter=N0,N1,N2  : comma-separate list of pass names or
    //                         or pass command args registered in passInfo.
    StringRef passNameList(IGC_GET_REGKEYSTRING(PrintAfter));
    StringRef PN = P->getPassName();
    if (IGCLLVM::equals_insensitive(passNameList, "all") || isInList(PN, passNameList))
      return true;

    // further check passInfo
    if (const PassInfo *PI = Pass::lookupPassInfo(P->getPassID())) {
      return isInList(PI->getPassArgument(), passNameList);
    }
  }
  return false;
}

void IGCPassManager::addPrintPass(Pass *P, bool isBefore) {
  auto passName = P->getPassName();
  std::string fullPassName = m_name + (isBefore ? "_before_" : "_after_") +
                             (passName.startswith("Unnamed pass") ? "UnnamedPass" : cleanPassName(passName.str()));

  auto name = IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
                  .ShaderName(m_pContext->shaderName)
                  .Type(m_pContext->type)
                  .Hash(m_pContext->hash)
                  .Pass(fullPassName, std::optional<uint32_t>(m_pContext->m_numPasses++))
                  .StagedInfo(m_pContext)
                  .Extension("ll");

  if (!name.allow())
    return;

  // The dump object needs to be on the Heap because it owns the stream, so
  // that the stream passed into the printer pass is valid during
  // PassManager::run() call.
  m_irDumps.emplace_front(name, IGC::Debug::DumpType::PASS_IR_TEXT);
  auto *stream = IGC_IS_FLAG_ENABLED(PrintMDBeforeModule) ? &m_irDumps.front().stream() : nullptr;
  PassManager::add(new SerializePrintMetaDataPass(stream));

  auto printerPass = P->createPrinterPass(m_irDumps.front().stream(), "");
  if (printerPass->getPassKind() == PT_Function) { // Enabling debug info for -O2
    delete printerPass;
    printerPass = llvm::createPrintModulePass(m_irDumps.front().stream(), "");
  }
  PassManager::add(printerPass);

  llvm::Pass *flushPass = createFlushPass(P, m_irDumps.front());
  if (nullptr != flushPass) {
    PassManager::add(flushPass);
  }
}

void DumpLLVMIR(IGC::CodeGenContext *pContext, const char *dumpName) {
  SetCurrentDebugHash(pContext->hash);
  SetCurrentEntryPoints(pContext->entry_names);
  auto makeName = [&]() {
    return DumpName(IGC::Debug::GetShaderOutputName())
        .ShaderName(pContext->shaderName)
        .Hash(pContext->hash)
        .Type(pContext->type)
        .Pass(dumpName)
        .Retry(pContext->m_retryManager->GetRetryId())
        .StagedInfo(pContext)
        .Extension("ll");
  };

  if (IGC_IS_FLAG_ENABLED(DumpLLVMIR)) {
    auto module = pContext->getModule();
    if (IGC_IS_FLAG_ENABLED(ShaderDumpInstNamer)) {
      // We first clear out any old entries of x if exists
      // from the ValueSymTab by overriding the x variable to _x
      for (auto &F : module->getFunctionList())
        for (BasicBlock &BB : F) {
          for (Instruction &I : BB)
            if (I.getName().startswith("x"))
              I.setName("_x");

          if (BB.getName().startswith("bb"))
            BB.setName("_bb");
        }
      // Now we rewrite the variables using a counter
      unsigned int counter = 1;
      unsigned int bb_counter = 0;
      for (auto &F : module->getFunctionList())
        for (BasicBlock &BB : F) {
          for (Instruction &I : BB) {
            if ((!I.hasName() && !I.getType()->isVoidTy()) || I.getName().startswith("_x")) {
              I.setName("x" + std::to_string(counter++));
            }
          }

          if (!BB.hasName() || BB.getName().startswith("_bb"))
            BB.setName("bb" + std::to_string(bb_counter++));
        }
    }

    pContext->getMetaDataUtils()->save(*pContext->getLLVMContext());
    serialize(*(pContext->getModuleMetaData()), module);

    auto new_annotator = IntrinsicAnnotator();
    auto annotator = (pContext->annotater != nullptr) ? pContext->annotater : &new_annotator;
    DumpLLVMIRText(module, makeName(), annotator);
  }
  if (IGC_IS_FLAG_ENABLED(ShaderOverride)) {
    SMDiagnostic Err;
    std::string fileName = makeName().overridePath();
    FILE *fp = fopen(fileName.c_str(), "r");
    if (fp != nullptr) {
      fclose(fp);
      // Check if not trying to override dump that cause a crash
      if (strcmp(dumpName, "codegen") == 0 || pContext->m_retryManager->GetRetryId() != 0) {
        pContext->EmitWarning(std::string("Override shader attempt detected for an unsupported dump: " + fileName +
                                          ". Skipping override.")
                                  .c_str());
        return;
      }
      errs() << "Override shader: " << fileName << "\n";
      Module *mod = parseIRFile(fileName, Err, *pContext->getLLVMContext()).release();
      if (mod) {
        pContext->deleteModule();
        pContext->setModule(mod);
        deserialize(*(pContext->getModuleMetaData()), mod);
        appendToShaderOverrideLogFile(fileName, "OVERRIDEN: ");
      } else {
        std::stringstream ss;
        ss << "Parse IR failed.\n";
        ss << Err.getLineNo() << ": " << Err.getLineContents().str() << "\n" << Err.getMessage().str() << "\n";

        std::string str = ss.str();
        errs() << str;
        appendToShaderOverrideLogFile(fileName, str.c_str());
      }
    }
  }
}

void DumpHashToOptions(const ShaderHash &hashs, const ShaderType type) {
  static std::mutex options_gen_mutex;

  // Enable for certain type(s) of shader(s)
  if ((1 << (uint32_t)type) & IGC_GET_FLAG_VALUE(GenerateOptionsFile)) {
    auto dump_file = IGC::Debug::DumpName("Options.txt").str();
    std::ostringstream content;
    std::ostringstream content_hash;
    QWORD hash = hashs.asmHash ? hashs.asmHash : hashs.psoHash;

    // build content
    if (!llvm::sys::fs::exists(dump_file)) {
      content
          << "# This file contains auto-generated shader hashes.\n"
          << "hash:";
    } else {
      content << ",";
    }

    content_hash << std::hex << "0x" << std::setfill('0') << std::setw(16) << hash << std::dec;
    options_gen_mutex.lock();
    // search for duplication, not generate if there is one already.
    bool hasDup = false;
    std::ifstream optionsInput(dump_file);

    if (optionsInput.is_open()) {
      std::string line;
      while (std::getline(optionsInput, line)) {
        if (std::string::npos != line.find(content_hash.str())) {
          hasDup = true;
          break;
        }
      }
      optionsInput.close();
    }

    if (!hasDup) {
      std::ofstream options(dump_file, std::ofstream::app);
      options << content.str() << content_hash.str();
      options.close();
    }

    options_gen_mutex.unlock();
  }
}

void InlineHelper::InlineAndOptimize(CallInst *callInst) {
#if defined(_RELEASE_INTERNAL) || defined(_DEBUG)
  // little check to make sure we are inlining the same function
  if (m_calledFunction)
    IGC_ASSERT(m_calledFunction == callInst->getCalledFunction());
  else
    m_calledFunction = callInst->getCalledFunction();
#endif
  auto *fn = callInst->getFunction();
  InlineFunctionInfo IFI;
  [[maybe_unused]] bool CanInline = IGCLLVM::InlineFunction(*callInst, IFI);
  IGC_ASSERT_MESSAGE(CanInline, "failed to inline?");

  // Merge static array allocas to reduce the use of private
  // memory. This is a similar optimization that exists in
  // the inliner, see mergeInlinedArrayAllocas().
  llvm::erase_if(IFI.StaticAllocas, [](AllocaInst *I) {
    return I->getAllocatedType()->isArrayTy() || !isa<ConstantInt>(I->getArraySize());
  });

  AllocaMap localAllocas;
  for (auto *I : IFI.StaticAllocas) {
    localAllocas[std::make_tuple(I->getType(), cast<ConstantInt>(I->getArraySize())->getZExtValue(),
                                 I->getAddressSpace())]
        .push_back(I);
  }

  for (auto &[key, perTypeAllocas] : localAllocas) {
    auto localAlloca = perTypeAllocas.begin();
    auto &globalAllocas = m_InlinedStaticArrayAllocas[fn][key];
    for (auto globalAlloca : globalAllocas) {
      (*localAlloca)->replaceAllUsesWith(globalAlloca);
      (*localAlloca)->eraseFromParent();

      localAlloca++;

      if (localAlloca == perTypeAllocas.end())
        break;
    }

    globalAllocas.append(localAlloca, perTypeAllocas.end());
  }
}
