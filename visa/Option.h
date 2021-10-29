/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _OPTION_H_
#define _OPTION_H_

#include <iostream>
#include <iomanip>
#include "visa_igc_common_header.h"
#include "VISAOptions.h"
#include "common.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

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
    EntryType getType(void) const { return type; }
    virtual void dump(void) const { std::cerr << "BASE"; }
    virtual ~VISAOptionsEntry() {}
};

struct VISAOptionsEntryBool : VISAOptionsEntry {
    VISAOptionsEntryBool(bool Val) {
        val.boolean = Val;
        type = ET_BOOL;
    }
    virtual void dump(void) const override {
        std::cerr << std::left << std::setw(10)
                  << ((val.boolean) ? "true" : "false");
    }
    bool getVal(void) const { return val.boolean; }
};
struct VISAOptionsEntryUint32 : VISAOptionsEntry {
    VISAOptionsEntryUint32(uint32_t Val) {
        val.int32 = Val;
        type = ET_INT32;
    }
    virtual void dump(void) const override {
        std::cerr << std::left << std::setw(10) << val.int32;
    }
    uint32_t getVal(void) const { return val.int32; }
};
struct VISAOptionsEntryUint64 : VISAOptionsEntry {
    VISAOptionsEntryUint64(uint64_t Val) {
        val.int64 = Val;
        type = ET_INT64;
    }
    virtual void dump(void) const override {
        std::cerr << std::left << std::setw(10) << val.int64;
    }
    uint64_t getVal(void) const { return val.int64; }
};
struct VISAOptionsEntryCstr : VISAOptionsEntry {
    VISAOptionsEntryCstr(const char *Val) {
        val.cstr = Val;
        type = ET_CSTR;
    }
    virtual void dump(void) const override {
        if (val.cstr) {
            std::cerr << std::left << std::setw(10) << val.cstr;
        } else {
            std::cerr << std::left << std::setw(10) << "NULL";
        }
    }
    const char *getVal(void) const { return val.cstr; }
};


class Options {
    std::unordered_map<std::string, vISAOptions> argToOption;
    const char *vISAOptionsToStr[vISA_NUM_OPTIONS];
    void initializeArgToOption(void);
    void initialize_vISAOptionsToStr(void);
    void initialize_m_vISAOptions(void);

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
    void getOption(vISAOptions option, const char*& buf) const;
    const char *getOptionCstr(vISAOptions option) const;
    uint32_t getuInt32Option(vISAOptions option) const;
    uint64_t getuInt64Option(vISAOptions option) const;

    void setTarget(VISATarget tTarget) { target = tTarget;}
    VISATarget getTarget() const { return target; }

    // APIs used by vISA clients (explicitly setting options)
    void setOption(vISAOptions option, bool val);
    void setOption(vISAOptions option, uint32_t val);
    void setOption(vISAOptions options, const char* str);

    // APIs used by vISA itself to set options internally
    void setOptionInternally(vISAOptions option, bool val);
    void setOptionInternally(vISAOptions option, uint32_t val);
    void setOptionInternally(vISAOptions options, const char* str);

    static void showUsage(std::ostream& output);

    std::stringstream& getUserArgString();
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
        VISAOptionsEntry *value;
        // This holds the default value
        VISAOptionsEntry *defaultValue;
        // The error message to show when argument is badly formed
        const char *errorMsg;
        // This is set to TRUE if this option is passed as an argument
        bool argIsSet;

        VISAOptionsLine(void) {
            argStr = (const char *) nullptr;
            type = ET_UNINIT;
            value = nullptr;
            defaultValue = nullptr;
            errorMsg = nullptr;
            argIsSet = false;
        }
        // Debug print
        void dump(void) const {
            std::cerr << std::setw(30) << argStr
                      << " [" << argIsSet << "] ";
            if (value) {
                value->dump();
            } else {
                std::cerr << std::left << std::setw(10) << "NULL";
            }
            std::cerr << ", (default:";
            if (defaultValue) {
                defaultValue->dump();
            } else {
                std::cerr << std::left << std::setw(10) << "NULL";
            }
            std::cerr << ")";
        }
    };

    // gcc 4.9.3 does not support enum as unordered_map key without an explicit hash function
    struct vISAOptionsHash
    {
        size_t operator()(vISAOptions opt) const
        {
            return (size_t)opt;
        }
    };

    // The main structure where we hold the options, their "-argument string",
    // their assigned values, their default values etc.
    // It is a map from vISAOptions->VISAOptionsLine
    class VISAOptionsDB {
    private:
        Options *options = nullptr;
        std::unordered_map<vISAOptions, VISAOptionsLine, vISAOptionsHash> optionsMap;
        // Check if KEY has already a value assigned to it
        void freeIfAlreadySet(vISAOptions key, bool dontCheckNull = true) {
            // If already set, free tha last one
            auto it = optionsMap.find(key);
            if (it != optionsMap.end()) {
                if (dontCheckNull || it->second.value != nullptr) {
                    delete it->second.value;
                }
            }
        }
    public:
        // Debug print all the options
        void dump(void) const {
            for (auto pair : optionsMap) {
                const VISAOptionsLine &line = pair.second;
                std::cerr << std::left << std::setw(34)
                          << options->get_vISAOptionsToStr(pair.first)
                          << ": ";
                line.dump();
                std::cerr << std::endl;
            }
        }
        // Debug print a single entry
        void dump(vISAOptions key) const {
            optionsMap.at(key).dump();
        }
        // If the option is passed as a command line argument
        void setArgSetByUser(vISAOptions key) {
            optionsMap[key].argIsSet = true;
        }
        // Set the value of the option
        void setBool(vISAOptions key, bool val) {
            freeIfAlreadySet(key);
            optionsMap[key].value = new VISAOptionsEntryBool(val);
        }
        void setUint32(vISAOptions key, uint32_t val) {
            freeIfAlreadySet(key);
            optionsMap[key].value = new VISAOptionsEntryUint32(val);
        }
        void setUint64(vISAOptions key, uint64_t val) {
            freeIfAlreadySet(key);
            optionsMap[key].value = new VISAOptionsEntryUint64(val);
        }
        void setCstr(vISAOptions key, const char *val) {
            freeIfAlreadySet(key, false);
            if (! val) {
                optionsMap[key].value = nullptr;
            } else {
                optionsMap[key].value = new VISAOptionsEntryCstr(val);
            }
        }

        // Set the value of the option
        void setDefaultBool(vISAOptions key, bool val) {
            optionsMap[key].defaultValue = new VISAOptionsEntryBool(val);
        }
        void setDefaultUint32(vISAOptions key, uint32_t val) {
            optionsMap[key].defaultValue = new VISAOptionsEntryUint32(val);
        }
        void setDefaultUint64(vISAOptions key, uint64_t val) {
            optionsMap[key].defaultValue = new VISAOptionsEntryUint64(val);
        }
        void setDefaultCstr(vISAOptions key, const char *val) {
            optionsMap[key].defaultValue = new VISAOptionsEntryCstr(val);
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

        // Get the argument string "-fooArg"
        const char *getArgStr(vISAOptions key) const {
            auto it = optionsMap.find(key);
            if (it != optionsMap.end()) {
                const char *argStr = it->second.argStr;
                return argStr;
            } else {
                return "UNDEFINED";
            }
        }

        // Get the type of KEY
        EntryType getType(vISAOptions key) const {
            assert(optionsMap.count(key));
            return optionsMap.at(key).type;
        }

        // Get the type of KEY
        const char *getErrorMsg(vISAOptions key) const {
            assert(optionsMap.count(key));
            return optionsMap.at(key).errorMsg;
        }


        // Get the values
        bool getBool(vISAOptions key) const {
            assert(optionsMap.count(key));
            const VISAOptionsEntry *value = optionsMap.at(key).value;
            assert(value->getType() == ET_BOOL && "Bad Type");
            const VISAOptionsEntryBool *Bool
                = static_cast<const VISAOptionsEntryBool *>(value);
            assert(Bool && "Uninitialized?");
            return Bool->getVal();
        }
        uint32_t getUint32(vISAOptions key) const {
            assert(optionsMap.count(key));
            const VISAOptionsEntry *value = optionsMap.at(key).value;
            assert(value->getType() == ET_INT32 && "Bad Type");
            const VISAOptionsEntryUint32 *Int32
                = static_cast<const VISAOptionsEntryUint32 *>(value);
            assert(Int32 && "Uninitialized?");
            return Int32->getVal();
        }
        uint64_t getUint64(vISAOptions key) const {
            assert(optionsMap.count(key));
            const VISAOptionsEntry *value = optionsMap.at(key).value;
            assert(value->getType() == ET_INT64 && "Bad Type");
            const VISAOptionsEntryUint64 *Int64
                = static_cast<const VISAOptionsEntryUint64 *>(value);
            assert(Int64 && "Uninitialized?");
            return Int64->getVal();
        }
        const char *getCstr(vISAOptions key) const {
            assert(optionsMap.count(key));
            const VISAOptionsEntry *value = optionsMap.at(key).value;
            if (! value) {
                return nullptr;
            }
            assert(value->getType() == ET_CSTR && "Bad Type");
            const VISAOptionsEntryCstr *Cstr
                = static_cast<const VISAOptionsEntryCstr *>(value);
            // assert(Cstr && "Bad option type OR uninit");
            return Cstr->getVal();
        }

        // TRUE if the options is passed as a cmd line argument
        bool isArgSetByUser(vISAOptions key) const {
            return optionsMap.at(key).argIsSet;
        }
        // Get defaults
        bool getDefaultBool(vISAOptions key) const {
            assert(optionsMap.count(key));
            const VISAOptionsEntry *defValue = optionsMap.at(key).defaultValue;
            assert(defValue->getType() == ET_BOOL && "Bad Type");
            const VISAOptionsEntryBool *Bool
                = static_cast<const VISAOptionsEntryBool *>(defValue);
            assert(Bool && "Uninitialized?");
            return Bool->getVal();
        }
        uint32_t getDefaultUint32(vISAOptions key) const {
            assert(optionsMap.count(key));
            const VISAOptionsEntry *defValue = optionsMap.at(key).defaultValue;
            assert(defValue->getType() == ET_INT32 && "Bad Type");
            const VISAOptionsEntryUint32 *Int32
                = static_cast<const VISAOptionsEntryUint32 *>(defValue);
            assert(Int32 && "Uninitialized?");
            return Int32->getVal();
        }
        uint64_t getDefaultUint64(vISAOptions key) const {
            assert(optionsMap.count(key));
            const VISAOptionsEntry *defValue = optionsMap.at(key).defaultValue;
            assert(defValue->getType() == ET_INT64 && "Bad Type");
            const VISAOptionsEntryUint64 *Int64
                = static_cast<const VISAOptionsEntryUint64 *>(defValue);
            assert(Int64 && "Uninitialized?");
            return Int64->getVal();
        }
        const char *getDefaultCstr(vISAOptions key) const {
            assert(optionsMap.count(key));
            const VISAOptionsEntry *defValue = optionsMap.at(key).defaultValue;
            assert(defValue->getType() == ET_CSTR && "Bad Type");
            const VISAOptionsEntryCstr *Cstr
                = static_cast<const VISAOptionsEntryCstr *>(defValue);
            assert(Cstr && "Uninitialized?");
            return Cstr->getVal();
        }
        VISAOptionsDB() {}
        VISAOptionsDB(Options *opt) {
            options = opt;
        }

        ~VISAOptionsDB(void) {
            for (auto pair : optionsMap) {
                auto *val = pair.second.value;
                delete val;
                auto *defVal = pair.second.defaultValue;
                delete defVal;

            }
        }
    };

    VISAOptionsDB m_vISAOptions;

    VISATarget target;

    // for debugging, store the options passed to command line/vISA builder
    std::stringstream argString;

    // legacy stepping setting for offline vISA compile.
    // FE compilers should not use this and should instead program the appropriate WATable/vISA option flags instead.
    Stepping stepping = Stepping::Step_none;

    int SetStepping(const char* str) {

        int retVal = VISA_SUCCESS;
        char upperchar = (char)std::toupper(*str);

        switch (upperchar)
        {
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
