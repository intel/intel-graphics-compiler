/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Option.h"
#include <optional>
#include "DebugInfo.h"
#include "IGC/common/StringMacros.hpp"
#include "PlatformInfo.h"
#include "Timer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string_view>

// NOTE: for now when adding a new flag, please also update ::reset() so that it
// is reset to the default value after the builder is destroyed.  We will move
// these options to be part of the builder in the near future.

static std::string makePlatformsString() {
  int n = 0;
  auto platforms = vISA::PlatformInfo::getGenxAllPlatforms(&n);
  std::stringstream ss;
  for (int i = 0; i < n; i++) {
    if (i > 0) {
      ss << ", ";
    }
    const char *name = vISA::PlatformInfo::getGenxPlatformString(platforms[i]);
    vASSERT(name);
    ss << name;
  }
  return ss.str();
}

// All arguments should be valid BE options.  When invoked from main the
// 0 argument which is a name of the program should be skipped when passed in.
//
// If the first argument passed in is a .isaasm/.visaasm file it will go
// through parser mode in which it will parse the .isaasm/.visaasm and create
// either regular or fat .isa file. GenX_IR can only accept a single
// .isaasm/.visaasm input on the command line.
//
// If first name that is passed in ends with *.isa it treats it as an offline
// compilation of .isa file.
bool Options::parseOptions(int argc, const char *argv[]) {
  int startPos = 0;
  vISA_ASSERT(!argToOption.empty(), "Must be initialized first!");

#define MAX_ARGC 128
  vISA_ASSERT(argc < MAX_ARGC, "too many options for vISA builder");

  for (int i = 0; i < argc; ++i) {
    argString << argv[i] << " ";
  }

  for (int i = startPos; i < argc; i++) {
    if (argv[i] == nullptr) {
      std::cerr << "INTERNAL ERROR: nullptr argv element\n";
      return false;
    }
    // If arg not defined in the .def file, exit with an error
    auto it = argToOption.find(argv[i]);
    if (it == argToOption.end()) {
      std::cerr << argv[i] << ": unrecognized option\n";
      showUsage(std::cout);
      return false;
    }
    bool parseError = false;
    auto parseU64 = [&](uint64_t &val) -> bool {
      char *end = nullptr;
      const char *arg = argv[i];
      uint64_t x = 0;
      if (arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
        x = std::strtoull(arg + 2, &end, 16);
      } else {
        x = std::strtoull(arg, &end, 10);
      }
      if (*end != 0) {
        parseError = true;
        std::cerr << it->first << ": " << argv[i] << ": malformed integer\n";
        return false;
      }
      val = x;
      return true;
    };
    auto parseU32 = [&](uint32_t &val) -> bool {
      uint64_t val64 = 0;
      if (!parseU64(val64))
        return false;
      if (val64 > (uint64_t)std::numeric_limits<uint32_t>::max()) {
        parseError = true;
        std::cerr << it->first << ": " << argv[i]
                  << ": malformed integer (value too large for 32b)\n";
        return false;
      }
      val = (uint32_t)val64;
      return true;
    };

    auto parseBool = [&](bool &val) -> bool {
      std::string_view arg(argv[i]);
      if (arg == "true" || arg == "TRUE") {
        val = true;
        return true;
      }
      if (arg == "false" || arg == "FALSE") {
        val = false;
        return true;
      }
      return false;
    };

    // If the current argv has a value, i.e., the next argv isn't a flag
    //   (flag starts with '-') treat it as either bool ("true" or "false") or a
    //   unsigned 32bit integer and return true if the value is non-zero;
    // otherwise, return false.
    //
    // Note that this is to make sure a boolean flag can be set true or
    // false explicitly. If no value, it will be inversed.
    auto parseOptionalBool = [&]() -> std::optional<bool> {
      const int next_i = i + 1;
      if (next_i >= argc)
        return std::nullopt;

      const char* arg = argv[next_i];
      if (arg[0] == '-')
        return std::nullopt;

      // advance i to the next argv : the value for this flag
      ++i;
      bool boolVal;
      if (parseBool(boolVal))
        return boolVal;
      uint32_t intVal;
      if (!parseU32(intVal)) {
        // Assume this flag does not have a value
        --i;
        return std::nullopt;
      }
      return intVal != 0;
    };

    // Arg corresponds to vISAOpt.
    // If bool:
    //  If argv[i+1] is either bool or int value, set the value to it.
    //  Otherwise, either set it to true or flip the default value depending on
    //  the option.
    // If int32,int64, or cstr, parse argv[i+1] and set the value.
    vISAOptions vISAOpt = it->second;
    EntryType type = m_vISAOptions.getType(vISAOpt);
    switch (type) {
    case ET_BOOL: {
      bool val;
      std::optional<bool> o = parseOptionalBool();
      if (o.has_value())
        val = o.value();
      else
        val = m_vISAOptions.getFlipBoolDefaultVal(vISAOpt)
            ? !m_vISAOptions.getDefaultBool(vISAOpt) : true;
      m_vISAOptions.setBool(vISAOpt, val);
      m_vISAOptions.setArgSetByUser(vISAOpt);
      break;
    }
    case ET_INT32:
    case ET_INT64:
    case ET_CSTR:
    case ET_2xINT32:
      i++;
      if (i >= argc) {
        const char *errorMsg = m_vISAOptions.getErrorMsg(vISAOpt);
        std::cerr << it->first << ": argument expected: " << errorMsg << "\n";
        return false;
      }
      switch (type) {
      case ET_INT32: {
        uint32_t val = 0;
        if (!parseU32(val)) {
          return false;
        }
        m_vISAOptions.setUint32(vISAOpt, val);
        m_vISAOptions.setArgSetByUser(vISAOpt);
        break;
      }
      case ET_INT64: {
        uint64_t val = 0;
        if (!parseU64(val)) {
          return false;
        }
        m_vISAOptions.setUint64(vISAOpt, val);
        m_vISAOptions.setArgSetByUser(vISAOpt);
        break;
      }
      case ET_CSTR:
        m_vISAOptions.setCstr(vISAOpt, argv[i]);
        m_vISAOptions.setArgSetByUser(vISAOpt);
        break;
      case ET_2xINT32: {
        uint32_t hi32 = 0;
        if (!parseU32(hi32)) {
          return false;
        }
        i++;
        if (i >= argc) {
          std::cerr << it->first
                    << ": vISA option type ET_2xINT32 requires two integer "
                       "arguments\n";
          return false;
        }
        uint32_t lo32 = 0;
        if (!parseU32(lo32)) {
          return false;
        }
        uint64_t val64 = ((uint64_t)hi32 << 32) | (uint64_t)lo32;
        m_vISAOptions.setUint64(vISAOpt, val64);
        m_vISAOptions.setArgSetByUser(vISAOpt);
        break;
      }
      default:
        vISA_ASSERT_UNREACHABLE("Bad option type");
      }
      break;
    default:
      vISA_ASSERT_UNREACHABLE("Bad option type");
    } // switch
    if (parseError)
      return false;
  }

  // Dependent arguments.
  // If setting an argument triggers more arguments, these should be
  // set here.
  if (m_vISAOptions.isArgSetByUser(vISA_Debug)) {
    m_vISAOptions.setBool(vISA_EnableSendFusion, false);
    m_vISAOptions.setBool(vISA_LocalScheduling, false);
    m_vISAOptions.setBool(vISA_Compaction, false);
    m_vISAOptions.setBool(vISA_LocalCopyProp, false);
    m_vISAOptions.setBool(vISA_LocalFlagOpt, false);
    m_vISAOptions.setBool(vISA_LocalMACopt, false);
    m_vISAOptions.setBool(vISA_LocalDefHoist, false);
    m_vISAOptions.setBool(vISA_LocalCleanMessageHeader, false);
    m_vISAOptions.setBool(vISA_forceNoMaskOnM0, false);
    m_vISAOptions.setBool(vISA_LocalRenameRegister, false);
    m_vISAOptions.setBool(vISA_FoldAddrImmed, false);
    m_vISAOptions.setBool(vISA_MergeScalar, false);
    m_vISAOptions.setBool(vISA_EnableMACOpt, false);
    m_vISAOptions.setBool(vISA_DisableleHFOpt, true);
    m_vISAOptions.setBool(vISA_LVN, false);
    m_vISAOptions.setBool(vISA_LocalRARoundRobin, false);
    m_vISAOptions.setBool(vISA_LocalBankConflictReduction, false);
    m_vISAOptions.setBool(vISA_RoundRobin, false);
    m_vISAOptions.setBool(vISA_SpiltLLR, false);
    m_vISAOptions.setBool(vISA_preRA_Schedule, false);
    m_vISAOptions.setBool(vISA_SplitGRFAlignedScalar, false);
    m_vISAOptions.setBool(vISA_SkipRedundantFillInRMW, false);
    m_vISAOptions.setBool(vISA_EnableDCE, false);
    m_vISAOptions.setBool(vISA_storeCE, true);
    m_vISAOptions.setBool(vISA_Debug, true);
  }
  if (m_vISAOptions.isArgSetByUser(vISA_Stepping)) {
    const char *stepping = m_vISAOptions.getCstr(vISA_Stepping);
    if (stepping == nullptr) {
      return false;
    }
    int status = SetStepping(stepping);
    if (status != 0) {
      std::cerr << "unrecognized stepping string: " << stepping << "\n";
      return false;
    }
  }
  if (m_vISAOptions.isArgSetByUser(vISA_DumpPasses)) {
    m_vISAOptions.setBool(vISA_DumpDotAll, true);
  }
  if (m_vISAOptions.isArgSetByUser(vISA_DumpDotAll)) {
    m_vISAOptions.setBool(vISA_DumpDot, true);
    m_vISAOptions.setBool(vISA_DumpSchedule, true);
    m_vISAOptions.setBool(vISA_DumpDagDot, true);
    m_vISAOptions.setBool(vISA_DumpPasses, true);
  }
  if (m_vISAOptions.isArgSetByUser(vISA_3DOption)) {
    target = VISA_3D;
  }

  if (m_vISAOptions.isArgSetByUser(vISA_ReservedGRFNum)) {
    if (m_vISAOptions.getUint32(vISA_ReservedGRFNum)) {
      m_vISAOptions.setBool(vISA_LocalBankConflictReduction, false);
    }
  }

  if (m_vISAOptions.isArgSetByUser(vISA_ForceSpills)) {
    m_vISAOptions.setBool(vISA_LocalRA, false);
  }

  if (m_vISAOptions.isArgSetByUser(vISA_HashVal)) {
    m_vISAOptions.setBool(vISA_InsertHashMovs, true);
  }

  if (m_vISAOptions.isArgSetByUser(vISA_DumpPerfStatsVerbose)) {
    m_vISAOptions.setBool(vISA_DumpPerfStats, true);
  }

  if (m_vISAOptions.isArgSetByUser(vISA_DumpPerfStats) ||
      m_vISAOptions.isArgSetByUser(vISA_DumpPerfStatsVerbose)) {
    m_vISAOptions.setBool(vISA_PrintRegUsage, true);
  }

  if (m_vISAOptions.isArgSetByUser(vISA_Platform)) {
    const char *platformStr = m_vISAOptions.getCstr(vISA_Platform);
    if (platformStr == nullptr) {
      return false;
    }

    TARGET_PLATFORM platform =
        vISA::PlatformInfo::getVisaPlatformFromStr(platformStr);
    if (platform == GENX_NONE) {
      std::cerr << platformStr << ": unrecognized platform string\n"
                << "supported platforms are: " << makePlatformsString() << "\n";
      return false;
    }
    m_vISAOptions.setUint32(vISA_PlatformSet, platform);
    if (GetStepping() == Step_A) {
      if (platform == GENX_TGLLP) {
        m_vISAOptions.setBool(vISA_HasEarlyGRFRead, true);
      }
    }
  }

  if (m_vISAOptions.isArgSetByUser(vISA_LabelStr)) {
    vISA_ASSERT(std::string_view(m_vISAOptions.getCstr(vISA_LabelStr)).size() <
                    MAX_LABEL_STR_LENGTH,
                "String length for unique labels is too long. Should be no "
                "larger than 8.");
    m_vISAOptions.setBool(vISA_UniqueLabels, true); // uniqueLabels = true;
  }
  if (m_vISAOptions.isArgSetByUser(VISA_AsmFileName)) {
    m_vISAOptions.setBool(vISA_AsmFileNameOverridden, true);
  }

  if (m_vISAOptions.isArgSetByUser(vISA_DisableSpillCoalescing)) {
    m_vISAOptions.setBool(vISA_DisableSpillCoalescing, true);
  }

  if (m_vISAOptions.isArgSetByUser(vISA_forceDebugSWSB)) {
    m_vISAOptions.setUint32(vISA_SWSBInstStall, 0);
    m_vISAOptions.setUint32(vISA_SWSBTokenBarrier, 0);
  }

  if (m_vISAOptions.isArgSetByUser(vISA_ForceSpills)) {
    m_vISAOptions.setBool(vISA_LinearScan, false);
  }

  if (m_vISAOptions.getBool(vISA_dumpVISAOptionsAll)) {
    dump();
  }

  return true;
}

void Options::showUsage(std::ostream &output) {
  output << "USAGE: GenX_IR <InputFilename.visaasm> {Option List}\n";
  output << "Converts a vISA assembly into Gen binary or assembly\n";
  output << "Options:\n";
#ifndef DLL_MODE
  output << "    -platform PLT                   - Gen platform to use "
            "(required)\n"
            "        supported platforms are: "
         << makePlatformsString()
         << "\n"
            "    -binary                         - Emit the binary code (Gen "
            "bits as .dat).\n";
#endif
  output
      << "    -output                         - Emit GEN assembly code to a "
         "file (.asm).\n"
         "    -dumpcommonisa                  - Emit CISA assembly "
         "(.visaasm).\n"
         "    -noschedule                     - Turn off code scheduling.\n"
         "    -nocompaction                   - Turn off binary compaction.\n"
         "    -outputIsaasmName <PATH>        - name for the combined .isaasm "
         "file.\n"
         "    -... many more; use -dumpVisaOptionsAll\n"
         "\n";
}

// This converts enum vISA_option to "vISA_option" so we can print it.
void Options::initialize_vISAOptionsToStr(void) {
#undef DEF_VISA_OPTION
#define DEF_VISA_OPTION(ENUM, TYPE, STR, ERROR_MSG, DEFAULT_VAL)               \
  vISAOptionsToStr[ENUM] = #ENUM;
#include "VISAOptionsDefs.h"
}

// Populate argToOption[] map
void Options::initializeArgToOption(void) {
#undef DEF_VISA_OPTION
#define DEF_VISA_OPTION(ENUM, TYPE, STR, ERROR_MSG, DEFAULT_VAL)               \
  argToOption[STR] = ENUM;
#include "VISAOptionsDefs.h"
}

// Initialize:
// 1. The strings "-someOption"
// 2. The default values
// 3. Set the value = default value
void Options::initialize_m_vISAOptions(void) {
  // I am not sure why, but without const_cast<> I am getting warnings
  // that "this" is const
  auto *Opts = const_cast<VISAOptionsDB *>(&m_vISAOptions);

#undef DEF_VISA_OPTION
#define DEF_VISA_OPTION(ENUM, TYPE, STR, ERROR_MSG, DEFAULT_VAL)               \
  Opts->setArgStr(ENUM, STR);                                                  \
  Opts->setType(ENUM, TYPE);                                                   \
  Opts->setErrorMsg(ENUM, ERROR_MSG);                                          \
  switch ((TYPE)) {                                                            \
  case ET_BOOL:                                                                \
    /* The casts are here to avoid type conversion warnings */                 \
    Opts->setDefaultBool(ENUM, (bool)(DEFAULT_VAL));                           \
    Opts->setBool(ENUM, (bool)(DEFAULT_VAL));                                  \
    break;                                                                     \
  case ET_BOOL_TRUE:                                                           \
    Opts->setType(ENUM, ET_BOOL);                                              \
    Opts->setDefaultBool(ENUM, (bool)(DEFAULT_VAL));                           \
    Opts->setBool(ENUM, (bool)(DEFAULT_VAL));                                  \
    Opts->setFlipBoolDefaultVal(ENUM, false);                                  \
    break;                                                                     \
  case ET_INT32:                                                               \
    /* The casts are here to avoid type conversion warnings */                 \
    Opts->setDefaultUint32(ENUM, (uint32_t)(uint64_t)(DEFAULT_VAL));           \
    Opts->setUint32(ENUM, (uint32_t)(uint64_t)(DEFAULT_VAL));                  \
    break;                                                                     \
  case ET_INT64:                                                               \
  case ET_2xINT32:                                                             \
    /* The casts are here to avoid type conversion warnings */                 \
    Opts->setDefaultUint64(ENUM, (uint64_t)(DEFAULT_VAL));                     \
    Opts->setUint64(ENUM, (uint64_t)(DEFAULT_VAL));                            \
    break;                                                                     \
  case ET_CSTR:                                                                \
    /* The casts are here to avoid type conversion warnings */                 \
    Opts->setDefaultCstr(ENUM, (const char *)(DEFAULT_VAL));                   \
    Opts->setCstr(ENUM, (const char *)(DEFAULT_VAL));                          \
    break;                                                                     \
  default:                                                                     \
    vISA_ASSERT_UNREACHABLE("Bad TYPE");                                       \
  }
#include "include/VISAOptionsDefs.h"
}

// This is to check whether an option of any type is set.
// It is useful for Cstr or Int arguments because getOption() will give us
// the value, not whether they are set or not.
bool Options::isOptionSetByUser(vISAOptions option) const {
  vISA_ASSERT(option < vISA_NUM_OPTIONS, "Option value is outside of range.");
  return m_vISAOptions.isArgSetByUser(option);
}

void Options::getOption(vISAOptions option, bool &value) const {
  vISA_ASSERT(option < vISA_NUM_OPTIONS, "Option value is outside of range.");
  value = m_vISAOptions.getBool(option);
}

bool Options::getOption(vISAOptions option) const {
  vISA_ASSERT(option < vISA_NUM_OPTIONS, "Option value is outside of range.");
  return m_vISAOptions.getBool(option);
}

void Options::getOption(vISAOptions option, const char *&buf) const {
  vISA_ASSERT(option < vISA_NUM_OPTIONS, "Option value is outside of range.");
  buf = m_vISAOptions.getCstr(option);
}

const char *Options::getOptionCstr(vISAOptions option) const {
  vISA_ASSERT(option < vISA_NUM_OPTIONS, "Option value is outside of range.");
  return m_vISAOptions.getCstr(option);
}

uint32_t Options::getuInt32Option(vISAOptions option) const {
  return m_vISAOptions.getUint32(option);
}

uint64_t Options::getuInt64Option(vISAOptions option) const {
  if (option == vISAOptions::vISA_HashVal)
    return m_vISAOptions.getUint64(vISA_HashVal);
  else if (option == vISAOptions::vISA_HashVal1)
    return m_vISAOptions.getUint64(vISA_HashVal1);

  return 0;
}

void Options::setOptionInternally(vISAOptions option, bool val) {
  m_vISAOptions.setBool(option, val);

#if (defined(_DEBUG) || defined(_INTERNAL))
  if (m_vISAOptions.getBool(vISA_dumpVISAOptions)) {
    std::cerr << std::setw(30) << vISAOptionsToStr[option] << " ";
    m_vISAOptions.dump(option);
    std::cerr << "\n";
  }
#endif
}

void Options::setOptionInternally(vISAOptions option, uint32_t value) {
  m_vISAOptions.setUint32(option, value);

#if (defined(_DEBUG) || defined(_INTERNAL))
  if (m_vISAOptions.getBool(vISA_dumpVISAOptions)) {
    std::cerr << std::setw(30) << vISAOptionsToStr[option] << " ";
    m_vISAOptions.dump(option);
    std::cerr << "\n";
  }
#endif
}

void Options::setOptionInternally(vISAOptions option, const char *str) {
  m_vISAOptions.setCstr(option, str);

#if (defined(_DEBUG) || defined(_INTERNAL))
  if (m_vISAOptions.getBool(vISA_dumpVISAOptions)) {
    std::cerr << std::setw(30) << vISAOptionsToStr[option] << " ";
    m_vISAOptions.dump(option);
    std::cerr << "\n";
  }
#endif
}

void Options::setOption(vISAOptions option, bool val) {
  m_vISAOptions.setArgSetByUser(option);
  setOptionInternally(option, val);
}

void Options::setOption(vISAOptions option, uint32_t value) {
  m_vISAOptions.setArgSetByUser(option);
  setOptionInternally(option, value);
}

void Options::setOption(vISAOptions option, const char *str) {
  m_vISAOptions.setArgSetByUser(option);
  setOptionInternally(option, str);
}

//
// return full set of arguments ever set by user, either through
// string options or the various setOptions()
//
std::string Options::getFullArgString() const {
  std::stringstream args;
  // Collect all user-set options.
  // This is for igc. When igc invokes vISA, it sets options
  // via setOption() api instead of options string, thus leave
  // argString empty. Here, we re-generate this options strings
  //
  // If a flag has no name (it happens), skip it!
  for (int i = vISA_OPTIONS_UNINIT + 1; i < vISA_NUM_OPTIONS; ++i) {
    vISAOptions o = (vISAOptions)i;
    if (isOptionSetByUser(o)) {
      EntryType type = m_vISAOptions.getType(o);
      switch (type) {
      case ET_BOOL:
        if (m_vISAOptions.getBool(o) != m_vISAOptions.getDefaultBool(o)) {
          // Boolean option means the reverse of the default!
          // (Probably should avoid such reverse handling)
          const char *argName = m_vISAOptions.getArgStr(o);
          if (argName && argName[0] != 0) {
            args << argName << " ";
          }
        }
        break;
      case ET_INT32: {
        const char *argName = m_vISAOptions.getArgStr(o);
        if (argName && argName[0] != 0 &&
          m_vISAOptions.getUint32(o) != m_vISAOptions.getDefaultUint32(o)) {
          args << argName << " " << m_vISAOptions.getUint32(o) << " ";
        }
        break;
      }
      case ET_INT64: {
        const char *argName = m_vISAOptions.getArgStr(o);
        if (argName && argName[0] != 0 &&
          m_vISAOptions.getUint64(o) != m_vISAOptions.getDefaultUint64(o)) {
          args << argName << " " << m_vISAOptions.getUint64(o) << " ";
        }
        break;
      }
      case ET_2xINT32: {
        uint32_t lo32, hi32;
        uint64_t val = m_vISAOptions.getUint64(o);
        lo32 = (uint32_t)val;
        hi32 = (uint32_t)(val >> 32);

        const char *argName = m_vISAOptions.getArgStr(o);
        if (argName && argName[0] != 0 &&
          val != m_vISAOptions.getDefaultUint64(o)) {
          args << argName << " " << hi32 << " " << lo32 << " ";
        }
      } break;
      case ET_CSTR: {
        const char *argName = m_vISAOptions.getArgStr(o);
        if (!argName || argName[0] == 0 ||
          m_vISAOptions.getCstr(o) == m_vISAOptions.getDefaultCstr(o)) {
          break;
        }
        args << argName << " ";
        const char *sval = m_vISAOptions.getCstr(o);
        // Careful not to emit empty strings
        // ... -string_opt   -next_opt
        //                 ^ empty string value for -string_opt
        // We need to put something so poor fool (me) that comes along
        // and tries to parse the .full_options line doesn't get
        // -string_opt parsing -next_opt as the value
        // Instead produce:
        // ... -string_opt ""  -next_opt"
        if (sval == nullptr || *sval == 0) {
          args << "\"\""; // emit escaped so tokenization in next parse works
        } else {
          std::string s = sval;
          if (s.find(' ') != std::string::npos) {
            // if there's spaces escape them
            //   -string_opt "foo bar" -next_opt
            args << "\"" << s << "\"";
          } else {
            args << s;
          }
        }
        args << " ";
        break;
      }
      default:
        vISA_ASSERT_UNREACHABLE("Invalid vISA option type!");
        args << "UNDEFINED ";
        break;
      }
    }
  }
  return args.str();
}

//
// this returns the options string explicitly passed in by user
//
std::stringstream &Options::getUserArgString() { return argString; }

std::string Options::getEncoderOutputFile() const {
  const char *encoderOutputFile = m_vISAOptions.getCstr(vISA_encoderFile);
  if (encoderOutputFile != nullptr) {
    return std::string(encoderOutputFile);
  } else {
    return std::string("");
  }
}

//
// This is to read visa options from environment variable: VISA_OPTIONS
// Options string is blank-seperated, just like ones used in genx_ir.
// For example,
//    VISA_OPTIONS=-dotAll -noAccSub
//  or
//    VISA_OPTIONS="-dotAll -noAccSub"
//
// The option in this environment variable overrides the previous one!
// This is intended for non visa standalone use only (not via genx_ir).
//
void Options::getOptionsFromEV() {
#if defined(_DEBUG) || defined(_INTERNAL)
  const char *visaOptionsEV = "VISA_OPTIONS";
  const char *pVisaEV = getenv(visaOptionsEV);
  if (!pVisaEV) {
    return;
  }

  std::cerr << "VISA Environment Variable in effect:\n"
            << "  (Note: if scalar IGC sets the same flag, it overrides one "
               "set by VISA_OPTIONS here!)\n"
            << visaOptionsEV << " = " << pVisaEV << "\n";

  std::string ostr(pVisaEV);
  // Remove leading/trailing quote if present
  size_t pos = ostr.find_first_not_of(' ');
  char firstC = ostr.at(pos);
  if (firstC == '"' || firstC == '\'') {
    ostr.at(pos) = ' ';
    size_t p0 = ostr.find_last_not_of(" ");
    if (firstC == ostr.at(p0)) {
      ostr.at(p0) = ' ';
    } else {
      std::cerr << "    Environment variable's leading and trailing quote ("
                << firstC << ", " << ostr.at(p0)
                << ") : not matched!  Ignored!\n";
      return;
    }
    // pos to the fist non-blank valid char
    pos = ostr.find_first_not_of(' ', pos + 1);
  }
  std::vector<std::string> flags;
  size_t currPos = pos;
  while (currPos != std::string::npos) {
    pos = ostr.find_first_of(' ', currPos);
    flags.emplace_back(ostr.substr(currPos, pos - currPos));
    currPos = ostr.find_first_not_of(' ', pos); // pos can be npos!
  }

  int sz = (int)flags.size();
  if (sz <= 0) {
    return;
  }
  char **argvars = new char *[sz];
  const char **pargs = (const char **)argvars;
  for (int i = 0; i < sz; ++i) {
    pargs[i] = flags[i].c_str();
  }
  parseOptions(sz, pargs);
  delete[] argvars;
  return;
#endif
}

void Options::dump(void) const { m_vISAOptions.dump(); }

Options::Options() : m_vISAOptions(this) {
  target = VISA_CM;

  initialize_vISAOptionsToStr();
  initializeArgToOption();
  initialize_m_vISAOptions();
}
