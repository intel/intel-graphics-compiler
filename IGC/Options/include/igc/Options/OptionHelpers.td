/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines helper object with common for all options utilities
// and common option types (ze, ocl or common options)

class ZeOption<string name> {
  string Name = !strconcat("ze-", name);
}

class ClOption<string name> {
  string Name = !strconcat("cl-", name);
}

// Helper for multiclasses to define aliasing with ze variant.
class CommonClAlias<string name> {
  Option Alias = !cast<Option>(name # _ze);
}

// Options without runtime prefixes.
class PlainFlag<string name> : Flag<["-"], name>;
class PlainJoined<string name> : Joined<["-"], name>;
class PlainSeparate<string name> : Separate<["-"], name>;
class PlainCommaJoined<string name> : CommaJoined<["-"], name>;
class PlainMultiArg<string name, int numargs> : MultiArg<["-"], name, numargs>;
class PlainJoinedOrSeparate<string name> : JoinedOrSeparate<["-"], name>;
class PlainJoinedAndSeparate<string name> : JoinedAndSeparate<["-"], name>;

// Level zero related option classes.
class ZeFlag<string name> : Flag<["-"], ZeOption<name>.Name>;
class ZeJoined<string name> : Joined<["-"], ZeOption<name>.Name>;
class ZeSeparate<string name> : Separate<["-"], ZeOption<name>.Name>;
class ZeCommaJoined<string name> : CommaJoined<["-"], ZeOption<name>.Name>;
class ZeMultiArg<string name, int numargs> : MultiArg<["-"], ZeOption<name>.Name, numargs>;
class ZeJoinedOrSeparate<string name> : JoinedOrSeparate<["-"], ZeOption<name>.Name>;
class ZeJoinedAndSeparate<string name> : JoinedAndSeparate<["-"], ZeOption<name>.Name>;

// OpenCL related option classes.
class ClFlag<string name> : Flag<["-"], ClOption<name>.Name>;
class ClJoined<string name> : Joined<["-"], ClOption<name>.Name>;
class ClSeparate<string name> : Separate<["-"], ClOption<name>.Name>;
class ClCommaJoined<string name> : CommaJoined<["-"], ClOption<name>.Name>;
class ClMultiArg<string name, int numargs> : MultiArg<["-"], ClOption<name>.Name, numargs>;
class ClJoinedOrSeparate<string name> : JoinedOrSeparate<["-"], ClOption<name>.Name>;
class ClJoinedAndSeparate<string name> : JoinedAndSeparate<["-"], ClOption<name>.Name>;

// Common for two runtimes option helpers.
multiclass CommonFlag<string name> {
def _ze : ZeFlag<name>;
def : ClFlag<name>, CommonClAlias<NAME>;
}
multiclass CommonJoined<string name> {
def _ze : ZeJoined<name>;
def : ClJoined<name>, CommonClAlias<NAME>;
}
multiclass CommonSeparate<string name> {
def _ze : ZeSeparate<name>;
def : ClSeparate<name>, CommonClAlias<NAME>;
}
multiclass CommonCommaJoined<string name> {
def _ze : ZeCommaJoined<name>;
def : ClCommaJoined<name>, CommonClAlias<NAME>;
}
multiclass CommonMultiArg<string name, int numargs> {
def _ze : ZeMultiArg<name, numargs>;
def : ClMultiArg<name, numargs>, CommonClAlias<NAME>;
}
multiclass CommonJoinedOrSeparate<string name> {
def _ze : ZeJoinedOrSeparate<name>;
def : ClJoinedOrSeparate<name>, CommonClAlias<NAME>;
}
multiclass CommonJoinedAndSeparate<string name> {
def _ze : ZeJoinedAndSeparate<name>;
def : ClJoinedAndSeparate<name>, CommonClAlias<NAME>;
}
