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

#ifndef GED_OPTION_PARSER_H
#define GED_OPTION_PARSER_H

# ifdef GED_BUILDING_EXECUTABLE

#include <string>
#include <set>
#include <map>

using std::string;
using std::set;
using std::map;


class OptionParser
{
public:
    // API FUNCTIONS

    inline OptionParser(const string& moduleName, const string& description) : _moduleName(moduleName), _description(description) {}
    inline ~OptionParser() { clear(); }

    void ParseOptions(int argc, const char* argv[]);

    void DefineOption(const char shortName, const string& longName, const string& description, const bool required, const bool multi,
                      const bool takesArgument, const string* defaultVal);
    void RequireOneOf(const set<string>& options);

    // Queries
    bool OptionSpecified(const string& option) const;
    const set<string>& GetOptionValues(const string& option) const;
    void Usage(string& usage) const;

private:
    // PRIVATE DATA TYPE DEFINITIONS

    struct ged_option_t
    {
        string _description;
        bool _multi;
        bool _takesArgument;
        set<string> _defaultVal;
        set<string> _values;
    };

private:
    // PRIVATE STATIC DATA MEMBERS
    static const size_t _maxLineLength;

private:
    // PRIVATE MEMBER FUNCTIONS

    void clear();
    void ValidateOptionNames(const char shortName, const string& longName);
    char ShortOption(const string& option) const;
    const string& LongOption(const char option) const;
    bool OptionDefined(const string& option) const;
    ged_option_t* FindOption(const string& option) const;

    void parseInputArg(const string& input, bool& needOptionArg, string& forOption);
    void HandleLongOption(const string& input, bool& needOptionArg, string& forOption);
    void HandleShortOption(const string& input, bool& needOptionArg, string& forOption);
    void HandleOptionArgument(const string& input, bool& needOptionArg, string& forOption);
    void ValidateRequiredOptions() const;

    void AddOneOptionUsage(string& usage, char shortOption, const ged_option_t* opt) const;

private:
    // PRIVATE DATA MEMBERS

    const string _moduleName;
    const string _description;
    map<char, ged_option_t*> _definedOptions;
    set<char> _requiredOptions;
    set<char> _specifiedOptions;
    set<set<char> > _requireOneOf;
    map<char, string> _shortToLong;
    map<string, char> _longToShort;
};

# endif // GED_BUILDING_EXECUTABLE

#endif // GED_OPTION_PARSER_H
