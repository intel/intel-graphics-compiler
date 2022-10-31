/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "iga_main.hpp"

#include <sstream>

bool listOps(const Opts &opts, const std::string &opmn) {
  bool hasError = false;
  auto ops =
      igax::OpSpec::enumerate(static_cast<igax::Platform>(opts.platform));
  std::stringstream ss;
  if (opmn.empty()) {
    // list a table of all ops
    // build header
    ss << std::setw(12) << std::left << "Mnemonic"
       << "  "
       << "Op"
       << "\n";
    for (const auto &op : ops) {
      ss << std::setw(12) << std::left << op.menmonic() << "  " << op.name()
         << "\n";
    }
  } else {
    bool prefixSearch = opmn[opmn.size() - 1] == '*';
    bool foundOp = false;
    for (auto &op : ops) {
      std::string mn = op.menmonic();
      if (opmn == mn || (prefixSearch && opmn.substr(0, opmn.size() - 1) ==
                                             mn.substr(0, opmn.size() - 1))) {
        foundOp = true;
        ss << "Mnemonic: " << mn << "\n";
        ss << "Name:     " << op.name() << "\n";
        ss << op.description() << "\n\n";
      }
    }
    hasError = !foundOp;
    if (!foundOp) {
      fatalExitWithMessage(opmn, ": mnemonic not found for platform");
    }
  }
  writeText(opts, ss.str().c_str());
  return hasError;
}
