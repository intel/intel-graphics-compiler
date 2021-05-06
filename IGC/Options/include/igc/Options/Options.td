/*========================== begin_copyright_notice ============================

Copyright (c) 2020-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// This file defines the options accepted by vector compiler.
//
// There are two kinds of options: api options and internal options.
//
// Api options are exposed to user via, e.g., openCL clBuildProgram.
//
// Internal options are for passing of additional info of various purposes.
// Among these can be: debug, things that are not exposed to user directly.
//
//===----------------------------------------------------------------------===//

include "llvm/Option/OptParser.td"

// Option kinds {{
// VC options accessible using public API.
def VCApiOption : OptionFlag;

// Api options compatible with igcmc.
// These are used only when -cmc is present in api options.
def IgcmcApiOption : OptionFlag;

// VC internal options.
def VCInternalOption : OptionFlag;
// }} Option kinds

// Helpers {{
def OptHelper {
  list<OptionFlag> VCBackwardCompatFlags = [VCApiOption, IgcmcApiOption];

  list<string> ZePrefixes = ["-ze-"];
  list<string> ClPrefixes = ["-cl-"];
  list<string> CommonPrefixes = !listconcat(ZePrefixes, ClPrefixes);
}

// Level zero related option classes.
class ZeFlag<string name> : Flag<OptHelper.ZePrefixes, name>;
class ZeJoined<string name> : Joined<OptHelper.ZePrefixes, name>;
class ZeSeparate<string name> : Separate<OptHelper.ZePrefixes, name>;
class ZeCommaJoined<string name> : CommaJoined<OptHelper.ZePrefixes, name>;
class ZeMultiArg<string name, int numargs> : MultiArg<OptHelper.ZePrefixes, name, numargs>;
class ZeJoinedOrSeparate<string name> : JoinedOrSeparate<OptHelper.ZePrefixes, name>;
class ZeJoinedAndSeparate<string name> : JoinedAndSeparate<OptHelper.ZePrefixes, name>;

// OpenCL related option classes.
class ClFlag<string name> : Flag<OptHelper.ClPrefixes, name>;
class ClJoined<string name> : Joined<OptHelper.ClPrefixes, name>;
class ClSeparate<string name> : Separate<OptHelper.ClPrefixes, name>;
class ClCommaJoined<string name> : CommaJoined<OptHelper.ClPrefixes, name>;
class ClMultiArg<string name, int numargs> : MultiArg<OptHelper.ClPrefixes, name, numargs>;
class ClJoinedOrSeparate<string name> : JoinedOrSeparate<OptHelper.ClPrefixes, name>;
class ClJoinedAndSeparate<string name> : JoinedAndSeparate<OptHelper.ClPrefixes, name>;

// Common for two runtimes option helpers.
class CommonFlag<string name> : Flag<OptHelper.CommonPrefixes, name>;
class CommonJoined<string name> : Joined<OptHelper.CommonPrefixes, name>;
class CommonSeparate<string name> : Separate<OptHelper.CommonPrefixes, name>;
class CommonCommaJoined<string name> : CommaJoined<OptHelper.CommonPrefixes, name>;
class CommonMultiArg<string name, int numargs> : MultiArg<OptHelper.CommonPrefixes, name, numargs>;
class CommonJoinedOrSeparate<string name> : JoinedOrSeparate<OptHelper.CommonPrefixes, name>;
class CommonJoinedAndSeparate<string name> : JoinedAndSeparate<OptHelper.CommonPrefixes, name>;
// }} Helpers

include "VCOptions.td"
