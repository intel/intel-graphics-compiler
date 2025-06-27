/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _OPTION_H_
#define _OPTION_H_

#include "Assertions.h"
#include "VISAOptions.h"
#include "common.h"
#include "visa_igc_common_header.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#define MAX_LABEL_STR_LENGTH 256

enum Stepping {
  Step_A = 0,
  Step_B = 1,
  Step_C = 2,
  Step_D = 3,
  Step_E = 4,
  Step_F = 5,
  Step_none = 6
};

enum EntryType {
  ET_UNINIT = 0,
  ET_BOOL,
  // like ET_BOOL, except that when specified without a value in command line
  // (e.g., "-foo"), it set the option to true instead of flipping its default
  // value.
  ET_BOOL_TRUE,
  ET_INT32,
  ET_2xINT32,
  ET_INT64,
  ET_CSTR,
  ET_MAX
};

union EntryValue {
  bool boolean;
  uint32_t int32;
  uint64_t int64;
  const char *cstr;
};

struct VISAOptionsEntry {
  EntryType type = ET_UNINIT;
  EntryValue val;
  EntryType getType() const { return type; }
  void setBool(bool Val) {
    val.boolean = Val;
    type = ET_BOOL;
  }
  void setUint32(uint32_t Val) {
    val.int32 = Val;
    type = ET_INT32;
  }
  void setUint64(uint64_t Val) {
    val.int64 = Val;
    type = ET_INT64;
  }
  void setCStr(const char *Val) {
    val.cstr = Val;
    type = ET_CSTR;
  }
  bool getBool() const {
    vASSERT(type == ET_BOOL);
    return val.boolean;
  }
  uint32_t getUint32() const {
    vASSERT(type == ET_INT32);
    return val.int32;
  }
  uint64_t getUint64() const {
    vASSERT(type == ET_INT64);
    return val.int64;
  }
  const char *getCStr() const {
    vASSERT(type == ET_CSTR);
    return val.cstr;
  }
  void dump() const {
    std::cerr << std::left << std::setw(10);
    if (type == ET_BOOL)
      std::cerr << ((val.boolean) ? "true" : "false");
    else if (type == ET_INT32)
      std::cerr << val.int32;
    else if (type == ET_INT64)
      std::cerr << val.int64;
    else if (type == ET_CSTR)
      std::cerr << (val.cstr ? val.cstr : "NULL");
    else
      std::cerr << "NULL";
  }
  virtual ~VISAOptionsEntry() {}
};

class Options {
  std::unordered_map<std::string, vISAOptions> argToOption;
  const char *vISAOptionsToStr[vISA_NUM_OPTIONS];
  void initializeArgToOption();
  void initialize_vISAOptionsToStr();
  void initialize_m_vISAOptions();

public:
  Options();

  const char *get_vISAOptionsToStr(vISAOptions opt) {
    return vISAOptionsToStr[opt];
  }
  bool parseOptions(int argc, const char *argv[]);
  // This is to check whether an option of any type is set.
  // It is useful for Cstr or Int arguments because getOption() will give us
  // the value, not whether they are set or not.
  bool isOptionSetByUser(vISAOptions option) const;

  void getOption(vISAOptions option, bool &value) const;
  bool getOption(vISAOptions option) const;
  void getOption(vISAOptions option, const char *&buf) const;
  const char *getOptionCstr(vISAOptions option) const;
  uint32_t getuInt32Option(vISAOptions option) const;
  uint64_t getuInt64Option(vISAOptions option) const;

  void setTarget(VISATarget tTarget) { target = tTarget; }
  VISATarget getTarget() const { return target; }

  // APIs used by vISA clients (explicitly setting options)
  void setOption(vISAOptions option, bool val);
  void setOption(vISAOptions option, uint32_t val);
  void setOption(vISAOptions options, const char *str);

  // APIs used by vISA itself to set options internally
  void setOptionInternally(vISAOptions option, bool val);
  void setOptionInternally(vISAOptions option, uint32_t val);
  void setOptionInternally(vISAOptions options, const char *str);

  static void showUsage(std::ostream &output);

  std::stringstream &getUserArgString();
  std::string getFullArgString() const;
  std::string getEncoderOutputFile() const;

  Stepping GetStepping() const { return stepping; }

  void getOptionsFromEV();

  void dump() const;

private:
  void setGTPin();

  // This holds the data of a single vISAOptions entry
  struct VISAOptionsLine {
    // This is the "-fooBarOption"
    const char *argStr;
    // The TYPE
    EntryType type;
    // This holds the actual value
    VISAOptionsEntry value;
    // This holds the default value
    VISAOptionsEntry defaultValue;
    // The error message to show when argument is badly formed
    const char *errorMsg;
    // This is set to TRUE if this option is passed as an argument
    bool argIsSet;
    // Behavior for a bool flag without value (e.g., "-foo"). TRUE means we flip
    // the default value, FALSE means we set the option to true.
    bool flipBoolDefaultVal;

    VISAOptionsLine() {
      argStr = (const char *)nullptr;
      type = ET_UNINIT;
      errorMsg = nullptr;
      argIsSet = false;
      flipBoolDefaultVal = true;
    }
    // Debug print
    void dump() const {
      std::cerr << std::setw(30) << argStr << " [" << argIsSet << "] ";
      value.dump();
      std::cerr << ", (default:";
      defaultValue.dump();
      std::cerr << ")";
    }
  };

  // gcc 4.9.3 does not support enum as unordered_map key without an explicit
  // hash function
  struct vISAOptionsHash {
    size_t operator()(vISAOptions opt) const { return (size_t)opt; }
  };

  // The main structure where we hold the options, their "-argument string",
  // their assigned values, their default values etc.
  // It is a vector of VISAOptionsLine indexed by the vISAOptions enum.
  class VISAOptionsDB {
  private:
    // Parent pointer.
    Options *options = nullptr;
    std::vector<VISAOptionsLine> optionsMap;

  public:
    // Debug print all the options
    void dump(void) const {
      for (int i = 0; i < static_cast<int>(vISA_NUM_OPTIONS); ++i) {
        const VISAOptionsLine &line = optionsMap[i];
        std::cerr << std::left << std::setw(34)
                  << options->get_vISAOptionsToStr(static_cast<vISAOptions>(i))
                  << ": ";
        line.dump();
        std::cerr << "\n";
      }
    }
    // Debug print a single entry
    void dump(vISAOptions key) const { optionsMap.at(key).dump(); }
    // If the option is passed as a command line argument
    void setArgSetByUser(vISAOptions key) { optionsMap[key].argIsSet = true; }
    // Set the value of the option
    void setBool(vISAOptions key, bool val) {
      optionsMap[key].value.setBool(val);
    }
    void setUint32(vISAOptions key, uint32_t val) {
      optionsMap[key].value.setUint32(val);
    }
    void setUint64(vISAOptions key, uint64_t val) {
      optionsMap[key].value.setUint64(val);
    }
    void setCstr(vISAOptions key, const char *val) {
      optionsMap[key].value.setCStr(val);
    }

    // Set the value of the option
    void setDefaultBool(vISAOptions key, bool val) {
      optionsMap[key].defaultValue.setBool(val);
    }
    void setDefaultUint32(vISAOptions key, uint32_t val) {
      optionsMap[key].defaultValue.setUint32(val);
    }
    void setDefaultUint64(vISAOptions key, uint64_t val) {
      optionsMap[key].defaultValue.setUint64(val);
    }
    void setDefaultCstr(vISAOptions key, const char *val) {
      optionsMap[key].defaultValue.setCStr(val);
    }

    // Set the "-fooBarOption"
    void setArgStr(vISAOptions key, const char *argStr) {
      optionsMap[key].argStr = argStr;
    }

    // Set the TYPE
    void setType(vISAOptions key, EntryType type) {
      optionsMap[key].type = type;
    }

    // Set the error message
    void setErrorMsg(vISAOptions key, const char *errorMsg) {
      optionsMap[key].errorMsg = errorMsg;
    }

    void setFlipBoolDefaultVal(vISAOptions key, bool val) {
      vASSERT(optionsMap[key].type == ET_BOOL);
      optionsMap[key].flipBoolDefaultVal = val;
    }

    bool getFlipBoolDefaultVal(vISAOptions key) const {
      return optionsMap[key].flipBoolDefaultVal;
    }

    // Get the argument string "-fooArg"
    const char *getArgStr(vISAOptions key) const {
      return optionsMap[key].argStr;
    }

    // Get the type of KEY
    EntryType getType(vISAOptions key) const { return optionsMap.at(key).type; }

    // Get the type of KEY
    const char *getErrorMsg(vISAOptions key) const {
      return optionsMap.at(key).errorMsg;
    }

    // Get the values
    // Note that value is guaranteed to be set already because we initialize all
    // vISAOptions' values to their default when constructing the Option object.
    bool getBool(vISAOptions key) const {
      const VISAOptionsEntry &value = optionsMap.at(key).value;
      return value.getBool();
    }
    uint32_t getUint32(vISAOptions key) const {
      const VISAOptionsEntry &value = optionsMap.at(key).value;
      return value.getUint32();
    }
    uint64_t getUint64(vISAOptions key) const {
      const VISAOptionsEntry &value = optionsMap.at(key).value;
      return value.getUint64();
    }
    const char *getCstr(vISAOptions key) const {
      const VISAOptionsEntry &value = optionsMap.at(key).value;
      return value.getCStr();
    }

    // TRUE if the options is passed as a cmd line argument
    bool isArgSetByUser(vISAOptions key) const {
      return optionsMap.at(key).argIsSet;
    }
    // Get defaults
    bool getDefaultBool(vISAOptions key) const {
      const VISAOptionsEntry &defValue = optionsMap.at(key).defaultValue;
      return defValue.getBool();
    }
    uint32_t getDefaultUint32(vISAOptions key) const {
      const VISAOptionsEntry &defValue = optionsMap.at(key).defaultValue;
      return defValue.getUint32();
    }
    uint64_t getDefaultUint64(vISAOptions key) const {
      const VISAOptionsEntry &defValue = optionsMap.at(key).defaultValue;
      return defValue.getUint64();
    }
    const char *getDefaultCstr(vISAOptions key) const {
      const VISAOptionsEntry &defValue = optionsMap.at(key).defaultValue;
      return defValue.getCStr();
    }
    explicit VISAOptionsDB(Options *opt)
        : options(opt), optionsMap(static_cast<int>(vISA_NUM_OPTIONS)) {}

    ~VISAOptionsDB() {}
  };

  VISAOptionsDB m_vISAOptions;

  VISATarget target;

  // for debugging, store the options passed to command line/vISA builder
  std::stringstream argString;

  // legacy stepping setting for offline vISA compile.
  // FE compilers should not use this and should instead program the appropriate
  // WATable/vISA option flags instead.
  Stepping stepping = Stepping::Step_none;

  int SetStepping(const char *str) {

    int retVal = VISA_SUCCESS;
    char upperchar = (char)std::toupper(*str);

    switch (upperchar) {
    case 'A':
      stepping = Step_A;
      break;
    case 'B':
      stepping = Step_B;
      break;
    case 'C':
      stepping = Step_C;
      break;
    case 'D':
      stepping = Step_D;
      break;
    case 'E':
      stepping = Step_E;
      break;
    case 'F':
      stepping = Step_F;
      break;
    default:
      // err msg?
      break;
    }
    return retVal;
  }
};

#endif
