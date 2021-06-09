/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifdef GED_BUILDING_EXECUTABLE

#include <sstream>
#include "common/ged_base.h"
#include "common/ged_string_utils.h"
#include "common/ged_option_parser.h"

using std::stringstream;


const size_t OptionParser::_maxLineLength = 80;


/*************************************************************************************************
 * class OptionParser API functions
 *************************************************************************************************/

void OptionParser::ParseOptions(int argc, const char* argv[])
{
    GEDASSERT(0 < argc);
    GEDASSERT(NULL != argv);
    bool needOptionArg = false;
    string forOption;
    for (int i = 1; i < argc; ++i)
    {
        GEDASSERT(NULL != argv[i]);
        parseInputArg(argv[i], needOptionArg, forOption);
    }
    ValidateRequiredOptions();
}


void OptionParser::DefineOption(const char shortName, const string& longName, const string& description, const bool required,
                                const bool multi, const bool takesArgument, const string* defaultVal)
{
    ValidateOptionNames(shortName, longName);
    if (!takesArgument && (NULL != defaultVal))
    {
        GEDERROR(shortName + string(" - Default value was specified but the option does not take an argument."));
    }

    ged_option_t* opt = new ged_option_t;
    opt->_description = description;
    opt->_multi = multi;
    opt->_takesArgument = takesArgument;
    if (takesArgument)
    {
        if (NULL != defaultVal)
        {
            opt->_defaultVal.insert(*defaultVal);
            GEDASSERT(1 == opt->_defaultVal.size());
        }
    }

    _definedOptions[shortName] = opt;
    if (!longName.empty())
    {
        _shortToLong[shortName] = longName;
        _longToShort[longName] = shortName;
    }

    if (required)
    {
        _requiredOptions.insert(shortName);
    }
}


void OptionParser::RequireOneOf(const set<string>& options)
{
    if (options.empty()) return;

    set<char> req;
    set<string>::const_iterator it = options.begin();
    const set<string>::const_iterator end = options.end();
    for (; it != end; ++it)
    {
        if (!OptionDefined(*it))
        {
            GEDERROR(string("Option ") + *it + " was not defined so it can't be required.");
        }
        char shortName = ShortOption(*it);
        GEDASSERT(0 != shortName);
        req.insert(shortName);
    }

    _requireOneOf.insert(req);
}


bool OptionParser::OptionSpecified(const string& option) const
{
    if (!OptionDefined(option)) return false; // no need to fail if the option was not defined
    if (1 == option.size())
    {
        return (0 != _specifiedOptions.count(option[0]));
    }
    else
    {
        GEDASSERTM(0 != _longToShort.count(option), string("Option ") + option + " was supposedly defined but was not found in the " +
                                                    "long-to-short name map.");
        return (0 != _specifiedOptions.count(_longToShort.find(option)->second));
    }
}


const set<string>& OptionParser::GetOptionValues(const string& option) const
{
    if (option.empty())
    {
        GEDERROR("Requesting values for an option with an empty name.");
    }

    const ged_option_t* opt = FindOption(option);
    if (NULL == opt)
    {
        GEDERROR(string("Option ") + option + " was not defined.");
    }

    if (!opt->_takesArgument)
    {
        GEDERROR(string("Option ") + option + " does not take an argument.");
    }

    if (!OptionSpecified(option))
    {
        if (!opt->_defaultVal.empty())
        {
            return opt->_defaultVal;
        }
        else
        {
            GEDERROR(string("Option ") + option + " was not specified and does not have a default value.");
        }
    }
    return opt->_values;
}


void OptionParser::Usage(string& usage) const
{
    // Add module description.
    if (!_description.empty())
    {
        vector<string> lines;
        BreakLines(_description, lines, _maxLineLength);
        GEDASSERT(0 < lines.size());
        usage = lines[0] + "\n";
        const size_t numOfLines = lines.size();
        for (size_t i = 1; i < numOfLines; ++i)
        {
            usage += lines[i] + "\n";
        }
        usage += "\n";
    }

    // Add module options.
    usage += "Usage: " + _moduleName + " [options]\n\n";
    map<char, ged_option_t*>::const_iterator it = _definedOptions.begin();
    const map<char, ged_option_t*>::const_iterator end = _definedOptions.end();
    for (; it != end; ++it)
    {
        AddOneOptionUsage(usage, it->first, it->second);
    }
}


/*************************************************************************************************
 * class OptionParser private member functions
 *************************************************************************************************/

void OptionParser::clear()
{
    _requiredOptions.clear();
    _specifiedOptions.clear();
    _requireOneOf.clear();
    _shortToLong.clear();
    _longToShort.clear();

    map<char, ged_option_t*>::iterator it = _definedOptions.begin();
    const map<char, ged_option_t*>::iterator end = _definedOptions.end();
    for (; it != end; ++it)
    {
        delete it->second;
    }
    _definedOptions.clear();
}


void OptionParser::ValidateOptionNames(const char shortName, const string& longName)
{
    if (1 == longName.size())
    {
        GEDERROR(longName + " - Long names must be longer than a single character.");
    }
    if (0 != _definedOptions.count(shortName))
    {
        GEDERROR(string("An option '") + shortName + "' already exists.");
    }
    GEDASSERT(0 == _requiredOptions.count(shortName));
    GEDASSERT(0 == _specifiedOptions.count(shortName));
    GEDASSERT(0 == _shortToLong.count(shortName));
    GEDASSERT(0 == _longToShort.count(longName));
}


char OptionParser::ShortOption(const string& option) const
{
    if (1 == option.size()) return option[0];

    const map<string, char>::const_iterator it = _longToShort.find(option);
    if (_longToShort.end() == it) return 0;
    return it->second;
}


const string& OptionParser::LongOption(const char option) const
{
    const map<char, string>::const_iterator it = _shortToLong.find(option);
    if (_shortToLong.end() == it) return emptyString;
    return it->second;
}


bool OptionParser::OptionDefined(const string& option) const
{
    if (option.empty()) return false;
    const char toFind = ShortOption(option);
    if (0 == toFind) return false;
    return (0 != _definedOptions.count(toFind));
}


OptionParser::ged_option_t* OptionParser::FindOption(const string& option) const
{
    GEDASSERT(!option.empty());

    const char toFind = ShortOption(option);
    if (0 == toFind) return NULL;

    const map<char, ged_option_t*>::const_iterator it = _definedOptions.find(toFind);
    if (_definedOptions.end() == it) return NULL;
    return it->second;
}


void OptionParser::parseInputArg(const string& input, bool& needOptionArg, string& forOption)
{
    GEDASSERT(input.size() > 0);
    if (0 == input.find("--"))
    {
        HandleLongOption(input, needOptionArg, forOption);
    }
    else if ('-' == input[0])
    {
        HandleShortOption(input, needOptionArg, forOption);
    }
    else
    {
        HandleOptionArgument(input, needOptionArg, forOption);
    }
}


void OptionParser::HandleLongOption(const string& input, bool& needOptionArg, string& forOption)
{
    const string longName = input.substr(2); // extract the option name without the prefix

    // Verify non empty option.
    if (longName.empty())
    {
        GEDBADOPTION("Invalid option '--'.");
    }

    // Make sure that we're not expecting an argument for the previous option.
    if (needOptionArg)
    {
        GEDBADOPTION(string("Expected an argument for '") + forOption + "' but got '" + input + "' instead.");
    }
    GEDASSERT(forOption.empty());

    // Get the option data structure.
    const ged_option_t* opt = FindOption(longName);
    if (NULL == opt)
    {
        GEDBADOPTION(string("Unknown option '") + input + "'.");
    }

    // Find the option's short name.
    const char shortName = _longToShort[longName];

    if (!opt->_multi)
    {
        if (0 != _specifiedOptions.count(shortName))
        {
            GEDBADOPTION(string("-") + shortName + " is only allowed once.");
        }
    }

    // Mark that the option was specified in the command line.
    _specifiedOptions.insert(shortName);

    // Prepare for receiving an argument if necessary.
    if (opt->_takesArgument)
    {
        needOptionArg = true;
        forOption = longName;
    }
}


void OptionParser::HandleShortOption(const string& input, bool& needOptionArg, string& forOption)
{
    // Verify non empty option.
    size_t inputLength = input.size();
    if (1 == inputLength)
    {
        GEDBADOPTION("Invalid option '-'.");
    }

    // Make sure that we're not expecting an argument for the previous option.
    if (needOptionArg)
    {
        GEDBADOPTION(string("Expected an argument for '") + forOption + "' but got '" + input + "' instead.");
    }
    GEDASSERT(forOption.empty());

    // Get the option data structure.
    char shortName = input[1];
    ged_option_t* opt = FindOption(input.substr(1, 1));
    if (NULL == opt)
    {
        GEDBADOPTION(string("Unknown option '-") + shortName + "'.");
    }

    if (!opt->_multi)
    {
        if (0 != _specifiedOptions.count(shortName))
        {
            GEDBADOPTION(string("-") + shortName + " is only allowed once.");
        }
    }

    // Mark that the option was specified in the command line.
    _specifiedOptions.insert(shortName);

    if (2 < inputLength) // an argument was concatenated (without a whitespace)
    {
        string optionArgument = input.substr(2);
        if (!opt->_takesArgument)
        {
            GEDBADOPTION(string("-") + shortName + " does not take an argument but '" + optionArgument + "' was given.");
        }
        opt->_values.insert(optionArgument);
    }
    else // check if the option takes an argument
    {
        if (opt->_takesArgument)
        {
            needOptionArg = true;
            forOption = shortName;
        }
    }
}


void OptionParser::HandleOptionArgument(const string& input, bool& needOptionArg, string& forOption)
{
    if (!needOptionArg)
    {
        GEDASSERT(forOption.empty());
        GEDBADOPTION(string("Expected an option, but received an option argument instead - '") + input + "'.");
    }
    GEDASSERT(!forOption.empty());
    ged_option_t* opt = FindOption(forOption);
    GEDASSERT(NULL != opt);
    opt->_values.insert(input);
    needOptionArg = false;
    forOption.clear();
}


void OptionParser::ValidateRequiredOptions() const
{
    set<char>::const_iterator reqIter = _requiredOptions.begin();
    const set<char>::const_iterator reqEnd = _requiredOptions.end();
    for (; reqIter != reqEnd; ++reqIter)
    {
        if (0 == _specifiedOptions.count(*reqIter))
        {
            GEDBADOPTION(string("-") + *reqIter + " is required but was not specified.");
        }
    }

    set<set<char> >::const_iterator setIter = _requireOneOf.begin();
    const set<set<char> >::const_iterator setEnd = _requireOneOf.end();
    for (; setIter != setEnd; ++setIter)
    {
        set<char>::const_iterator oneOfIter = (*setIter).begin();
        const set<char>::const_iterator oneOfEnd = (*setIter).end();
        bool found = false;
        for (; oneOfIter != oneOfEnd; ++oneOfIter)
        {
            if (0 != _specifiedOptions.count(*oneOfIter))
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            oneOfIter = (*setIter).begin();
            stringstream strm;
            strm << "One of [-" << *oneOfIter;
            for (++oneOfIter; oneOfIter != oneOfEnd; ++oneOfIter)
            {
                strm << "|-" << *oneOfIter;
            }
            strm << "] was expected but none were specified.";
            GEDBADOPTION(strm.str());
        }
    }
}


void OptionParser::AddOneOptionUsage(string& usage, const char shortOption, const ged_option_t* opt) const
{
    static const string tab(indentationFactor, ' ');

    string optionStr = "-" + string(1, shortOption);
    const string& longOption = LongOption(shortOption);
    if (!longOption.empty())
    {
        optionStr += ", --" + longOption;
    }
    if (opt->_takesArgument)
    {
        optionStr += " ARG";
    }

    optionStr += "\n";
    const size_t lineLength = _maxLineLength - indentationFactor;
    string optionDesc = opt->_description;
    if (opt->_multi)
    {
        optionDesc += " This option may be specified multiple times.";
    }
    if (!opt->_defaultVal.empty())
    {
        optionDesc += " (default: " + *(opt->_defaultVal.begin()) + ")";
    }
    vector<string> lines;
    BreakLines(optionDesc, lines, lineLength);
    const size_t numberOfLines = lines.size();
    for (size_t i = 0; i < numberOfLines; ++i)
    {
        optionStr += tab + lines[i] + "\n";
    }
    usage += optionStr;
}

#endif // GED_BUILDING_EXECUTABLE
