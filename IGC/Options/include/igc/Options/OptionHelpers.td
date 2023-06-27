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
  Option Alias = !cast<Option>(name # _common);
}

// Options without runtime prefixes.
class PlainFlag<string name> : Flag<["-"], name>;
class PlainJoined<string name> : Joined<["-"], name>;
class PlainSeparate<string name> : Separate<["-"], name>;
class PlainJoinedOrSeparate<string name> : JoinedOrSeparate<["-"], name>;

// Level zero -ze option classes.
class ZeFlag<string name> : Flag<["-"], ZeOption<name>.Name>;
class ZeJoined<string name> : Joined<["-"], ZeOption<name>.Name>;
class ZeSeparate<string name> : Separate<["-"], ZeOption<name>.Name>;

// Level zero -ze-intel option classes.
class ZeIntelFlag<string name> : Flag<["-"], ZeIntelOption<name>.Name>;
class ZeIntelJoined<string name> : Joined<["-"], ZeIntelOption<name>.Name>;
class ZeIntelSeparate<string name> : Separate<["-"], ZeIntelOption<name>.Name>;

// Level zero -ze-opt option classes.
class ZeOptFlag<string name> : Flag<["-"], ZeOptOption<name>.Name>;
class ZeOptJoined<string name> : Joined<["-"], ZeOptOption<name>.Name>;
class ZeOptSeparate<string name> : Separate<["-"], ZeOptOption<name>.Name>;

// OpenCL -cl option classes.
class ClFlag<string name> : Flag<["-"], ClOption<name>.Name>;
class ClJoined<string name> : Joined<["-"], ClOption<name>.Name>;
class ClSeparate<string name> : Separate<["-"], ClOption<name>.Name>;

// OpenCL -cl-intel option classes.
class ClIntelFlag<string name> : Flag<["-"], ClIntelOption<name>.Name>;
class ClIntelJoined<string name> : Joined<["-"], ClIntelOption<name>.Name>;
class ClIntelSeparate<string name> : Separate<["-"], ClIntelOption<name>.Name>;


// Common for two runtimes option helpers.
multiclass CommonFlag<string name> {
def _common : ZeFlag<name>;
def : ZeIntelFlag<name>, CommonAlias<NAME>;
def : ZeOptFlag<name>, CommonAlias<NAME>;
def : ClFlag<name>, CommonAlias<NAME>;
def : ClIntelFlag<name>, CommonAlias<NAME>;
def : PlainFlag<name>, CommonAlias<NAME>;
}

multiclass CommonJoined<string name> {
def _common : ZeJoined<name>;
def : ZeIntelJoined<name>, CommonAlias<NAME>;
def : ZeOptJoined<name>, CommonAlias<NAME>;
def : ClJoined<name>, CommonAlias<NAME>;
def : ClIntelJoined<name>, CommonAlias<NAME>;
def : PlainJoined<name>, CommonAlias<NAME>;
}

multiclass CommonSeparate<string name> {
def _common : ZeSeparate<name>;
def : ZeIntelSeparate<name>, CommonAlias<NAME>;
def : ZeOptSeparate<name>, CommonAlias<NAME>;
def : ClSeparate<name>, CommonAlias<NAME>;
def : ClIntelSeparate<name>, CommonAlias<NAME>;
def : PlainSeparate<name>, CommonAlias<NAME>;
}
