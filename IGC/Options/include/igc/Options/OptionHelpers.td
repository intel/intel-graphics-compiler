/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines helper object with common for all options utilities
// and common option types (ze, ocl or common options)

class ZeOption<string name> {
  string Name = !strconcat("ze-", name);
}

class ZeIntelOption<string name> {
  string Name = !strconcat("ze-intel-", name);
}

class ZeOptOption<string name> {
  string Name = !strconcat("ze-opt-", name);
}

class ClOption<string name> {
  string Name = !strconcat("cl-", name);
}

class ClIntelOption<string name> {
  string Name = !strconcat("cl-intel-", name);
}

// Helper for multiclasses to define aliasing with ze variant.
class CommonAlias<string name> {
  Option Alias = !cast<Option>(name # __common);
}

// Options without runtime prefixes.
class PlainFlag<string name> : Flag<["-"], name>;
class PlainJoined<string name> : Joined<["-"], name>;
class PlainSeparate<string name> : Separate<["-"], name>;
class PlainCommaJoined<string name> : CommaJoined<["-"], name>;
class PlainMultiArg<string name, int numargs> : MultiArg<["-"], name, numargs>;
class PlainJoinedOrSeparate<string name> : JoinedOrSeparate<["-"], name>;
class PlainJoinedAndSeparate<string name> : JoinedAndSeparate<["-"], name>;

// Level zero -ze option classes.
class ZeFlag<string name> : Flag<["-"], ZeOption<name>.Name>;
class ZeJoined<string name> : Joined<["-"], ZeOption<name>.Name>;
class ZeSeparate<string name> : Separate<["-"], ZeOption<name>.Name>;
class ZeCommaJoined<string name> : CommaJoined<["-"], ZeOption<name>.Name>;
class ZeMultiArg<string name, int numargs> : MultiArg<["-"], ZeOption<name>.Name, numargs>;
class ZeJoinedOrSeparate<string name> : JoinedOrSeparate<["-"], ZeOption<name>.Name>;
class ZeJoinedAndSeparate<string name> : JoinedAndSeparate<["-"], ZeOption<name>.Name>;


// Level zero -ze-intel option classes.
class ZeIntelFlag<string name> : Flag<["-"], ZeIntelOption<name>.Name>;
class ZeIntelJoined<string name> : Joined<["-"], ZeIntelOption<name>.Name>;
class ZeIntelSeparate<string name> : Separate<["-"], ZeIntelOption<name>.Name>;
class ZeIntelCommaJoined<string name> : CommaJoined<["-"], ZeIntelOption<name>.Name>;
class ZeIntelMultiArg<string name, int numargs> : MultiArg<["-"], ZeIntelOption<name>.Name, numargs>;
class ZeIntelJoinedOrSeparate<string name> : JoinedOrSeparate<["-"], ZeIntelOption<name>.Name>;
class ZeIntelJoinedAndSeparate<string name> : JoinedAndSeparate<["-"], ZeIntelOption<name>.Name>;

// Level zero -ze-opt option classes.
class ZeOptFlag<string name> : Flag<["-"], ZeOptOption<name>.Name>;
class ZeOptJoined<string name> : Joined<["-"], ZeOptOption<name>.Name>;
class ZeOptSeparate<string name> : Separate<["-"], ZeOptOption<name>.Name>;
class ZeOptCommaJoined<string name> : CommaJoined<["-"], ZeOptOption<name>.Name>;
class ZeOptMultiArg<string name, int numargs> : MultiArg<["-"], ZeOptOption<name>.Name, numargs>;
class ZeOptJoinedOrSeparate<string name> : JoinedOrSeparate<["-"], ZeOptOption<name>.Name>;
class ZeOptJoinedAndSeparate<string name> : JoinedAndSeparate<["-"], ZeOptOption<name>.Name>;

// OpenCL -cl option classes.
class ClFlag<string name> : Flag<["-"], ClOption<name>.Name>;
class ClJoined<string name> : Joined<["-"], ClOption<name>.Name>;
class ClSeparate<string name> : Separate<["-"], ClOption<name>.Name>;
class ClCommaJoined<string name> : CommaJoined<["-"], ClOption<name>.Name>;
class ClMultiArg<string name, int numargs> : MultiArg<["-"], ClOption<name>.Name, numargs>;
class ClJoinedOrSeparate<string name> : JoinedOrSeparate<["-"], ClOption<name>.Name>;
class ClJoinedAndSeparate<string name> : JoinedAndSeparate<["-"], ClOption<name>.Name>;

// OpenCL -cl-intel option classes.
class ClIntelFlag<string name> : Flag<["-"], ClIntelOption<name>.Name>;
class ClIntelJoined<string name> : Joined<["-"], ClIntelOption<name>.Name>;
class ClIntelSeparate<string name> : Separate<["-"], ClIntelOption<name>.Name>;
class ClIntelCommaJoined<string name> : CommaJoined<["-"], ClIntelOption<name>.Name>;
class ClIntelMultiArg<string name, int numargs> : MultiArg<["-"], ClIntelOption<name>.Name, numargs>;
class ClIntelJoinedOrSeparate<string name> : JoinedOrSeparate<["-"], ClIntelOption<name>.Name>;
class ClIntelJoinedAndSeparate<string name> : JoinedAndSeparate<["-"], ClIntelOption<name>.Name>;

// Common for two runtimes option helpers.
multiclass CommonFlag<string name> {
def __common : ClFlag<name>;
def : ClIntelFlag<name>, CommonAlias<NAME>;
def : ZeFlag<name>, CommonAlias<NAME>;
def : ZeIntelFlag<name>, CommonAlias<NAME>;
def : ZeOptFlag<name>, CommonAlias<NAME>;
}
multiclass CommonJoined<string name> {
def __common : ClJoined<name>;
def : ClIntelJoined<name>, CommonAlias<NAME>;
def : ZeJoined<name>, CommonAlias<NAME>;
def : ZeIntelJoined<name>, CommonAlias<NAME>;
def : ZeOptJoined<name>, CommonAlias<NAME>;
}
multiclass CommonSeparate<string name> {
def __common : ClSeparate<name>;
def : ClIntelSeparate<name>, CommonAlias<NAME>;
def : ZeSeparate<name>, CommonAlias<NAME>;
def : ZeIntelSeparate<name>, CommonAlias<NAME>;
def : ZeOptSeparate<name>, CommonAlias<NAME>;
}
multiclass CommonCommaJoined<string name> {
def __common : ClCommaJoined<name>;
def : ClIntelCommaJoined<name>, CommonAlias<NAME>;
def : ZeCommaJoined<name>, CommonAlias<NAME>;
def : ZeIntelCommaJoined<name>, CommonAlias<NAME>;
def : ZeOptCommaJoined<name>, CommonAlias<NAME>;
}
multiclass CommonMultiArg<string name, int numargs> {
def __common : ClMultiArg<name, numargs>;
def : ClIntelMultiArg<name, numargs>, CommonAlias<NAME>;
def : ZeMultiArg<name, numargs>, CommonAlias<NAME>;
def : ZeIntelMultiArg<name, numargs>, CommonAlias<NAME>;
def : ZeOptMultiArg<name, numargs>, CommonAlias<NAME>;
}
multiclass CommonJoinedOrSeparate<string name> {
def __common : ClJoinedOrSeparate<name>;
def : ClIntelJoinedOrSeparate<name>, CommonAlias<NAME>;
def : ZeJoinedOrSeparate<name>, CommonAlias<NAME>;
def : ZeIntelJoinedOrSeparate<name>, CommonAlias<NAME>;
def : ZeOptJoinedOrSeparate<name>, CommonAlias<NAME>;
}
multiclass CommonJoinedAndSeparate<string name> {
def __common : ZeJoinedAndSeparate<name>;
def : ClIntelJoinedAndSeparate<name>, CommonAlias<NAME>;
def : ZeJoinedAndSeparate<name>, CommonAlias<NAME>;
def : ZeIntelJoinedAndSeparate<name>, CommonAlias<NAME>;
def : ZeOptJoinedAndSeparate<name>, CommonAlias<NAME>;
}
