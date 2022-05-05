/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _OPTS_HPP_
#define _OPTS_HPP_

#include <cstdio>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "fatal.hpp"
#include "io.hpp"

#define OPTS_INVALID_SPEC(M) \
    fatalExitWithMessage( \
        "INTERNAL ERROR: INVALID SPEC given to " \
        "opts.hpp\n" #M)

namespace opts {

static bool streq(const char *str1, const char *str2) {
    return str1 == str2 || (str1 && str2 && strcmp(str1, str2) == 0);
}
static bool strpfx(const char *pfx, const char *str) {
    return strncmp(str, pfx, strlen(pfx)) == 0;
}
static bool parseInt(const char *inp, int &val)
{
    try {
        int radix = 10;
        bool negate = false;
        if (inp[0] == '-') {
            negate = true;
            inp++;
        }
        if (inp[0] && inp[0] == '0' && inp[1] &&
            (inp[1] == 'x' || inp[1] == 'X'))
        {
            // if we detect hex, change the radix to 16
            radix = 16;
            inp += 2;
        }
        char *end = nullptr;
        int pval = (int)std::strtol(inp, &end, radix);
        if (*end) {
            // some sort of suffix
            return false;
        }
        if (negate)
            pval = -pval;
        val = pval;
    } catch(...) {
        return false;
    }
    return true;
}

// takes message and handles the error; second arg is an optional usage message
// typedef std::function<void(const char *, const char *)> error_handler_t;
struct ErrorHandler {
    const char *exeName;
    const char *optName;

    ErrorHandler(const char *exe, const char *opt)
        : exeName(exe), optName(opt) { }

    void fail(const std::string &msg) const {
        std::string s;
        if (optName) {
            s += optName;
            s += ": ";
        }
        s += msg;
        fatalMessage(s.c_str());
        fatalExit();
    }
    void operator()(const std::string &msg) const {
        fail(msg);
    }

    int parseInt(const char *inp) const {
        int x = -1;
        if (!opts::parseInt(inp, x)) {
            std::stringstream ss;
            ss << inp << ": malformed int";
            fail(ss.str());
        }
        return x;
    }
};

static std::string concat(
    const std::string &a,
    const std::string &b,
    const std::string &c = "",
    const std::string &d = "",
    const std::string &e = "") {
    std::stringstream ss;
    ss << a << b << c << d << e;
    return ss.str();
}

// sets a value in the options set
template<typename O> using Setter =
    std::function<void(const char *, const ErrorHandler &, O &)>;

// sets option or arg to it's default value
template<typename O> using DefaultSetter =
    std::function<void(const ErrorHandler &, O &)>;

enum OptAttrs {
    NONE        = 0x00,
    ALLOW_MULTI = 0x01, // option can be specified multiple times
    ALLOW_UNSET = 0x02, // not an error if the option is never set
    FLAG = 0x04, // option takes no argument (setValue will be passed nullptr)
    OPT_FLAG_VAL = 0x08, // optional flag value: for stuff like the -h option
                         // which can be -h or -h=foo
    //  HIDDEN = 0x10        // for a hidden option (hidden from help message)
};

static inline int maxInt(int a, int b) {
    return a >= b ? a : b;
}

// An option specification specifies all the info about an option
// It has lambda functions for parsing and setting the value as well as
// specifying a default value.
//
// Type parameter is for the actual options structure.
template <typename O>
struct Opt {
    const char *groupPrefix;
    const char *shortName;
    const char *longName;
    const char *typeName;
    const char *description;
    const char *extendedDescription;
    const char *group;

    Setter<O>         setValue;   // called to set a value
    DefaultSetter<O>  setDefault; // called when a option is unspecified

    int timesMatched;

    OptAttrs attributes;

    // constructor option specification with default
    Opt(const char *g,
        const char *s,
        const char *l,
        const char *t,
        const char *d,
        const char *xd,
        int attrs, // OptAttrs
        Setter<O> setVal,
        DefaultSetter<O> setDflt)
        : groupPrefix(g),
          shortName(s),
          longName(l),
          typeName(t),
          description(d),
          extendedDescription(xd),
          attributes((OptAttrs)attrs),
          setValue(setVal),
          setDefault(setDflt),
          timesMatched(0) {}

    // constructor for option specification without default
    Opt(const char *g,
        const char *s,
        const char *l,
        const char *t,
        const char *d,
        const char *xd,
        int attrs, // OptAttrs
        Setter<O> setVal)
        : groupPrefix(g),
          shortName(s),
          longName(l),
          typeName(t),
          description(d),
          extendedDescription(xd),
          setValue(setVal),
          setDefault(noDefault()),
          timesMatched(0),
          attributes((OptAttrs)attrs)
        {}

    DefaultSetter<O> noDefault() const {
        std::string optStr = optName();
        return [=](const ErrorHandler &eh, O &) {
            eh(concat(optStr, " undefined"));
        };
    }

    bool hasAttribute(OptAttrs attr) const {
        return (attributes & attr) != 0;
    }

    // options have a non-null short or long name
    bool isArg() const { return !shortName && !longName; }
    // arguments have both null long and short names
    bool isOpt() const { return !isArg(); }

    bool tryMatch(
        int argc,
        const char **argv,
        int &argIx,
        const ErrorHandler &errHandler,
        O &opts)
    {
        std::string msgHelp  = makeHelpMessage();

        const char *token = argv[argIx];
        const char *value = nullptr;

        ErrorHandler eh(errHandler.exeName, token);

        if (isOpt()) {
            if (token[0] != '-')
                return false;

            // group prefix; e.g. -Xfoo
            int off = 1;
            if (groupPrefix) {
                if (!strpfx(groupPrefix, token + 1)) {
                    return false;
                }
                off += 1;
            }

            const char *key;
            if (shortName && token[0] == '-' &&
                strpfx(shortName, token + off)) {
                // short option -f=... or -f ...
                key   = token + off;
                value = key + strlen(shortName);
            } else if (
                longName && token[0] == '-' && token[1] == '-' &&
                strpfx(longName, token + off + 1)) {
                // long option --foo=... or --foo ...
                key   = token + off + 1;
                value = key + strlen(longName);
            } else {
                // no match
                return false;
            }

            if (*value == 0) {
                // -flag
                // OR
                // -key value
                if (hasAttribute(FLAG)) {
                    // -flag
                    value = nullptr;
                } else if (argIx == argc - 1) {
                    if (hasAttribute(OPT_FLAG_VAL)) {
                        value = nullptr;
                    } else {
                        eh("unexpected end of command line");
                    }
                } else if (hasAttribute(OPT_FLAG_VAL) &&
                    argv[argIx + 1][0] == '-')
                {
                    // -v -p=...
                    //  ^
                    // since next starts -, we
                    value = nullptr;
                } else {
                    // not a flag and we have tokens left
                    // next token is the value for this option
                    argIx++;
                    value = argv[argIx];
                }
            } else if (*value == '=') {
                // of the form -key=value
                if (hasAttribute(FLAG) && !hasAttribute(OPT_FLAG_VAL)) {
                    eh("option is a flag");
                } else {
                    value++; // step past =
                }
            } else {
                // junk at end of key (recall, we strncmp'd)
                // e.g. this option is "--foo" and the token is "--food"
                // this is just a mismatch (no harm done)
                return false;
            }

            // ensure the option hasn't been specified before
            if (timesMatched > 0 && !hasAttribute(OptAttrs::ALLOW_MULTI)) {
                eh("respecification");
            }
        } else { // isArg()
            if (timesMatched > 0 && !hasAttribute(OptAttrs::ALLOW_MULTI)) {
                // arg that may only be specified once, skip it (there might
                // be another arg, so it's not an error)
                return false;
            }
            value = token;
        } // end isOpt() / isArg()

        // attempt to parse the input, specialize the error handler to
        // this option/argument
        setValue(value, eh, opts);
        timesMatched++;

        // move past key or value to next token
        argIx++;
        return true;
    }

    void appendHelpMessage(
        std::ostream &os,
        int sCw = 0,
        int lCw = 0,
        int tCw = 0,
        bool appendExtDesc = true) const
    {
        // if column width is unspecified, then rescale it based on the
        // actual value's lengths
        const char *groupName = groupPrefix ? groupPrefix : "";
        int grLen = (int)strlen(groupName);

        if (isOpt()) {
            // -X....               type   desc
            //           --X....    type   desc
            // -X....    --X....    type   desc
            if (shortName) {
                os << "  -" << std::setw(grLen) << groupName;
                os << std::left << std::setw(sCw) << shortName;
            } else {
                os << "   " << std::setw(grLen) << "";
                os << std::setw(sCw) << "";
            }
            if (longName) {
                os << "  --" << std::setw(grLen) << groupName;
                os << std::left << std::setw(lCw) << longName;
            } else {
                os << "    " << std::setw(grLen) << "";
                os << std::setw(lCw) << "";
            }
        }

        std::string typeNameStr = typeName ? typeName : "";
        if (isArg() && hasAttribute(ALLOW_MULTI))
            typeNameStr += (hasAttribute(ALLOW_UNSET) ? '*' : '+');
        else
            typeNameStr += ' ';
        os << "  " << std::left << std::setw(tCw + 1) << typeNameStr; //+1 for + or *
        if (description)
            os << "  " << std::left << description;
        if (appendExtDesc && extendedDescription) {
            os <<  "\n";
            // auto format the description (crudely for now)
            size_t col = 1, slen = strlen(extendedDescription);
            auto canBreakHere = [&](size_t k) {
                return extendedDescription[k] == ' ' ||
                  extendedDescription[k] == '\n' ||
                  extendedDescription[k] == '-' ||
                  extendedDescription[k] == ',' ||
                  extendedDescription[k] == '.' ||
                  extendedDescription[k] == ';';
            };
            auto emitChar = [&](char c) {
              os << c;
              col ++;
              if (c == '\n')
                col = 1;
            };
            // TODO: determine the max cols via console/tty functions
            static const size_t MAX_COLS = 80;
            size_t i = 0;
            while (i < slen) {
                while (i < slen && isspace(extendedDescription[i])) {
                    emitChar(extendedDescription[i++]);
                }
                // find the next span
                size_t ext = 1;
                while (i + ext < slen && !canBreakHere(i + ext))
                  ext++;
                // if it doesn't fit, put a newline in
                if (col + ext >= MAX_COLS)
                  emitChar('\n');
                // emit the span
                for (size_t k = 0; k < ext; k++)
                  emitChar(extendedDescription[i + k]);
                i += ext;
            }
        }
    }

    // convenience accessor (for errors)
    std::string makeHelpMessage(
        int sCw            = 0,
        int lCw            = 0,
        int tCw            = 0,
        bool appendExtDesc = true) const {
        std::stringstream ss;
        appendHelpMessage(ss, sCw, lCw, tCw, appendExtDesc);
        return ss.str();
    }

    std::string optName() const {
        std::string str;
        if (isArg()) {
            str += "argument ";
        } else {
            str += "option ";
        }
        if (shortName) {
            str += "-";
            str += shortName;
        } else if (longName) {
            str = "--";
            str += longName;
        }
        return str;
    }
};

// Specifies a group of options all starting with the same prefix
// E.g. experimental options might start with "X", compiler warnings
// might be enabled or disabled with -Winexact-values syntax
template <typename O>
struct Group {
    const char *prefix; // e.g. "X"
    const char *name;   // e.g. "Experimental Options"
    std::vector<Opt<O>> members;

    Group(const char *prefix_, const char *name_)
        : prefix(prefix_), name(name_) {}
    Group(Group &&copy) {
        prefix  = copy.prefix;
        name    = copy.name;
        members = std::move(copy.members);
    }
    Group(const Group &copy) = delete;
    Group operator=(const Group &) = delete;

    // a flag requires no argument, but allows for one
    void defineFlag(
        const char *sNm,
        const char *lNm,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        Setter<O> setVal)
    {
        // TODO: validateOptNames(sNm, lNm); have to link to parent
        validateOptPrefix(sNm);
        validateOptPrefix(lNm);
        Opt<O> temp(prefix, sNm, lNm, "", desc, extDesc, attrs | FLAG, setVal);
        members.emplace_back(temp);
    }
    void defineFlag(
        const char *sNm,
        const char *lNm,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        bool &value)
    {
        defineFlag(sNm, lNm, desc, extDesc, attrs,
            [&] (const char *, const ErrorHandler &, O &) {
                value = true;
            });
    }

    void defineOpt(
        const char *sNm,
        const char *lNm,
        const char *type,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        Setter<O> setVal)
    {
        // TODO: validateOptNames(sNm, lNm); have to link to parent
        validateOptPrefix(sNm);
        validateOptPrefix(lNm);
        Opt<O> temp(prefix, sNm, lNm, type, desc, extDesc, attrs, setVal);
        members.emplace_back(temp);
    }
    void defineOpt(
        const char *sNm,
        const char *lNm,
        const char *type,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        Setter<O> setVal,
        DefaultSetter<O> setDftVal)
    {
        validateOptPrefix(sNm);
        validateOptPrefix(lNm);
        Opt<O> temp(
            prefix, sNm, lNm, type, desc, extDesc, attrs, setVal, setDftVal);
        members.emplace_back(temp);
    }
    void defineOpt(
        const char *sNm,
        const char *lNm,
        const char *type,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        std::string &stringValue)
    {
        define(sNm, lNm, type, desc, extDesc, attrs,
            [&] (const char *inp, const ErrorHandler &, O &) {
                stringValue = inp;
            });
    }

    bool tryMatch(
        int argc,
        const char **argv,
        int &argIx,
        const ErrorHandler &errHandler,
        O &opts)
    {
        for (auto &o : members) {
            if (o.tryMatch(argc, argv, argIx, errHandler, opts)) {
                return true;
            }
        }
        return false;
    }

    void validateOptPrefix(const char *nm)
    {
#ifdef _DEBUG
        if (nm == nullptr)
            return;
        // ensure they didn't prefix their own -'s
        if (*nm == '-') {
            OPTS_INVALID_SPEC(
                "option should not start with (the '-' is implict)");
        }
        // ensure doesn't collide with main name
        for (auto &o : members) {
            if (o.shortName && streq(nm, o.shortName)) {
                OPTS_INVALID_SPEC(
                    "option name collides with another (short) option name");
            }
            if (o.longName && streq(nm, o.longName)) {
                OPTS_INVALID_SPEC(
                    "option name collides with another (long) option name");
            }
        }
#endif
    }

    void appendGroupSummary(std::ostream &os) const {
        if (prefix) {
            os << "OPTION GROUP " << prefix << "\n";
        }
        int sCw = 4, lCw = 1, tCw = 1;
        for (const auto &o : members) {
            auto updateMax = [&] (int cw, const char *str) {
                return str ? maxInt(cw, (int)strlen(str)) : cw;
            };
            sCw = updateMax(sCw, o.shortName);
            lCw = updateMax(lCw, o.longName);
            tCw = updateMax(tCw, o.typeName);
        }
        for (const auto &o : members) {
            o.appendHelpMessage(os, sCw, lCw, tCw, false);
            os << "\n";
        }
    }
};

template <typename O>
class CmdlineSpec {
    const char *exeTitle;
    const char *exeName;
    Group<O> opts;                     // command line options (top-level)
    std::vector<Group<O> *> optGroups; // special option groups (e.g. -X....)
    std::vector<Opt<O>> args;          // command line arguments
    const char *examples;

  public:
    CmdlineSpec(
        const char *title,
        const char *exe,
        const char *examps = "",
        bool appendHelpOpt = true)
        : exeTitle(title)
        , exeName(exe)
        , opts(nullptr, nullptr)
        , examples(examps)
    {
        if (appendHelpOpt) {
            defineFlag(
                "h",
                "help",
                "shows help on an option",
                "Without any argument -h will print general help on all options.  However, "
                "if given an argument, -h will attempt to lookup that argument.  Options are "
                "given without preceding - or --.  We check long option names first, short "
                "names second, and finally argument indices last.  Argument indices are "
                "specified as: \"#1\" for the first argument, \"#2\" for the second, etc."
                "\n"
                "EXAMPLES:\n"
                "  % iga64 -h=p\n"
                "lists information on the -p option\n"
                "  % iga64 -h=Xfoo\n"
                "lists information on the -Xfoo option\n"
                "  % iga64 -h=#1\n"
                "lists information on the first argument (non-option)\n"
                "",
                OptAttrs::ALLOW_UNSET | OptAttrs::OPT_FLAG_VAL,
                [&] (const char *inp, const ErrorHandler &err, O &) {
                    CmdlineSpec::handleHelpArgument(
                        inp,
                        err,
                        exeTitle,
                        exeName,
                        opts,
                        optGroups,
                        args,
                        examples);
                }); // end  defineFlag(...)
        }           // end if appendHelpOpt
    }               // end constructor

    // a flag requires no argument, but allows for one
    void defineFlag(
        const char *sNm,
        const char *lNm,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        Setter<O> setter)
    {
        opts.defineFlag(sNm, lNm, desc, extDesc, attrs, setter);
    }
    void defineFlag(
        const char *sNm,
        const char *lNm,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        bool &value)
    {
        defineFlag(sNm, lNm, desc, extDesc, attrs,
            [&] (const char *, const ErrorHandler &, O &) {
                value = true;
            });
    }

    void defineOpt(
        const char *sNm,
        const char *lNm,
        const char *type,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        Setter<O> setter)
    {
        opts.defineOpt(sNm, lNm, type, desc, extDesc, attrs, setter);
    }

    void defineOpt(
        const char *sNm,
        const char *lNm,
        const char *type,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        Setter<O> setter,
        DefaultSetter<O> defaultSetter)
    {
        opts.defineOpt(
            sNm, lNm, type, desc, extDesc, attrs, setter, defaultSetter);
    }

    void defineOpt(
        const char *sNm,
        const char *lNm,
        const char *type,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        std::string &stringValue)
    {
        defineOpt(sNm, lNm, type, desc, extDesc, attrs,
            [&] (const char *inp, const ErrorHandler &, O &) {
                stringValue = inp;
            });
    }

    void defineArg(
        const char *type,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        Setter<O> setter)
    {
        Opt<O> temp(
            nullptr, nullptr, nullptr, type, desc, extDesc, attrs, setter);
        args.emplace_back(temp);
    }

    void defineArg(
        const char *type,
        const char *desc,
        const char *extDesc,
        int attrs, // OptAttrs
        Setter<O> setter,
        DefaultSetter<O> defaultSetter)
    {
        Opt<O> temp(
            nullptr,
            nullptr,
            nullptr,
            type,
            desc,
            extDesc,
            attrs,
            setter,
            defaultSetter);
        args.emplace_back(temp);
    }

    Group<O> &defineGroup(const char *prefix, const char *name) {
#ifdef _DEBUG
        if (prefix) {
            // ensure they didn't prefix their own -'s
            if (*prefix == '-') {
                OPTS_INVALID_SPEC(
                    "group prefix should not start with (the '-' is implicit)");
            }
            // ensure name doesn't collide with another top-level option name
            opts.validateOptPrefix(prefix);
            // ensure name doesn't collide with another group prefix
            for (auto &g : optGroups) {
                if (g->prefix && streq(prefix, g->prefix)) {
                    OPTS_INVALID_SPEC(
                        "group prefix collides with another group name");
                }
            }
        }
#endif
        optGroups.emplace_back(new Group<O>(prefix, name));
        return *optGroups.back();
    }

    void handleHelpArgument(
        const char *inp,
        const ErrorHandler &err,
        const char *exeTitle,
        const char *exeName,
        const Group<O> &opts,
        const std::vector<Group<O> *> &groups,
        const std::vector<Opt<O>> &args,
        const char *examples)
    {
        if (!inp || !*inp) {
            // no input given e.g. -h
            if (exeTitle)
                std::cerr << exeTitle << "\n";
            CmdlineSpec::appendUsage(
                std::cerr,
                opts,
                groups,
                args,
                exeName,
                examples);
            exit(EXIT_SUCCESS);
        } else {
            // given input: e.g. -h foo -h #1
            while (*inp == '-')
              inp++; // convert -h=-foo -> -h=foo
            const Opt<O> *opt = nullptr;
            auto scanGroup = [&](const Group<O> &g, bool pfx) {
                int off = 0;
                if (g.prefix) {
                    if (streq(g.prefix, inp)) {
                        // exact group match; bail and deal with it below
                        return;
                    } else if (strpfx(g.prefix, inp)) {
                        // group prefix e.g. "Xfoo" for group "X"
                        off += (int)strlen(g.prefix);
                    }
                }
                auto match = pfx ? strpfx : streq;
                for (auto &o : g.members) {
                    // try long names
                    if (o.longName && match(inp + off, o.longName)) {
                        opt = &o;
                        break;
                    }
                    // try short names
                    if (o.shortName && match(inp + off, o.shortName)) {
                        opt = &o;
                        break;
                    }
                }
            };

            // try for exact option matches first (prefers exact over
            // partial matches)
            scanGroup(opts, false);
            if (!opt) {
                for (auto &g : groups) {
                    scanGroup(*g, false);
                }
            }
            // try for an exact group match first
            // Group<O> *group = nullptr;
            for (auto g : groups) {
                if (streq(g->prefix, inp)) {
                    g->appendGroupSummary(std::cerr);
                    exit(EXIT_SUCCESS);
                }
            }
            // then fall back to prefix matches on options
            if (!opt) {
                scanGroup(opts, true);
                if (!opt) {
                    for (auto &g : groups) {
                        scanGroup(*g, true);
                    }
                }
            }
            // no prefix match on groups

            // TODO: refactor this to use std::vector and keep all matches
            // that way we can deal with ambiguity and emit a
            // "did you mean ..." message
            if (opt) {
                opt->appendHelpMessage(std::cerr, 0, 0, 0, true);
                exit(EXIT_SUCCESS);
            } else if (inp[0] == '#') {
                // try as an argument
                // #1 #2 ... are the args
                int argIx = -1;
                if (opts::parseInt(inp + 1, argIx)) {
                    if (argIx - 1 < 0 || argIx - 1 >= (int)args.size()) {
                        err("-h option: invalid argument index");
                    } else {
                        args[argIx - 1].appendHelpMessage(
                            std::cerr, 0, 0, 0, true);
                        exit(EXIT_SUCCESS);
                    }
                } // else e.g. "#1abc"
            } else {
                std::string str =
                    "-h=...: passed unknown option, argument, or group ";
                err(str + inp);
            }
        } // end if *inp != 0
    }

    bool parse(int argc, const char **argv, O &optVal) {
        int argIx = 1;

        // no arguments given ==> -h
        if (argIx == argc) {
            static const char *help[2] = {argv[0], "-h"};
            argc                       = sizeof(help) / sizeof(help[0]);
            argv                       = help;
        }

        ErrorHandler errHandler(exeName, nullptr);
        while (argIx < argc) {
            bool matched = false;

            // try as global opt
            matched = opts.tryMatch(argc, argv, argIx, errHandler, optVal);
            if (!matched) {
                // try as grouped option
                for (auto &g : optGroups) {
                    if (g->tryMatch(argc, argv, argIx, errHandler, optVal)) {
                        matched = true;
                        break;
                    }
                }
            }

            // try as a group name match (-X is alias for -h=X)
            if (!matched && argv[argIx][0] == '-') {
                for (auto &g : optGroups) {
                    if (streq(argv[argIx] + 1, g->prefix)) {
                        // recurse
                        std::string helpArg = std::string("-h=") + g->prefix;
                        static const char *helpArgs[2] =
                            {argv[0], helpArg.c_str()};
                        return parse(2, helpArgs, optVal);
                    }
                }
            }

            // try as arg
            if (!matched && argv[argIx][0] != '-') {
                for (auto &a : args) {
                    if (a.tryMatch(argc, argv, argIx, errHandler, optVal)) {
                        matched = true;
                        break;
                    }
                }
            }

            // failed to match
            if (!matched) {
                std::string msg = argv[argIx];
                msg += ": unmatched ";
                if (argv[argIx][0] == '-') {
                    msg += "option";
                } else {
                    msg += "argument";
                }
                errHandler(msg);
                return false;
            }
        }

        // set defaults and check for missing
        auto setDefaults = [&](Group<O> &g) {
            for (auto &o : g.members) {
                if (o.timesMatched == 0) {
                    if (!o.hasAttribute(OptAttrs::ALLOW_UNSET)) {
                        o.setDefault(errHandler, optVal);
                        //  } else {
                        //    errHandler(concat(
                        //      o.optName().c_str(),
                        //      ": undefined\n",
                        //      o.makeHelpMessage().c_str()));
                    }
                }
            }
        };
        setDefaults(opts);
        for (auto &g : optGroups) {
            setDefaults(*g);
        }

        for (auto &o : args) {
            if (o.timesMatched == 0) {
                if (!o.hasAttribute(OptAttrs::ALLOW_UNSET)) {
                    o.setDefault(errHandler, optVal);
                }
            }
        }

        return true;
    }

    static void appendUsage(
        std::ostream &os,
        const Group<O> &opts,
        const std::vector<Group<O> *> &groups,
        const std::vector<Opt<O>> &args,
        const char *exeName,
        const char *examples)
    {
        os << "usage: " << exeName << " OPTIONS ARGS\n";
        os << "where OPTIONS:\n";
        // autoscale all options
        opts.appendGroupSummary(os);
        int gCw = 4;
        for (const auto &g : groups) {
            if (g->prefix) {
                gCw = maxInt(gCw, (int)strlen(g->prefix) + 3);
            }
        }
        os << "\n";
        for (const auto &g : groups) {
            os << std::setw(3 + gCw) << std::left
               << concat("  -", g->prefix, "...");
            os << "  "; // spaces for option long names: should be lCw
            os << "  " << g->name << " (-" << g->prefix << " for more info)\n";
        }

        os << "and where ARGS:\n";
        // autoscale arg type column
        int atCw = 22;
        for (const auto &a : args) {
            atCw = maxInt(atCw, a.typeName ? (int)strlen(a.typeName) : 0);
        }
        for (const auto &a : args) {
            a.appendHelpMessage(os, gCw, 0, atCw, false);
            os << "\n";
        }
        if (examples) {
            os << "\n" << "EXAMPLES:\n"
                << examples;
        }
    }
}; // class CmdlineSpec

static const char *exeName(const char *path) {
    const char *sfx = path + strlen(path) - 1;
    while (sfx > path) {
        if (*sfx == '/' || *sfx == '\\')
            return sfx + 1;
        sfx--;
    }
    return path;
}
} // namespace opts

#endif // _OPTS_HPP_
