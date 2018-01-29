
#include "iga_main.hpp"


bool listOps(const Opts &opts, const std::string &opmn)
{
    bool hasError = false;
    auto ops = igax::OpSpec::enumerate(static_cast<igax::Platform>(opts.platform));
    std::stringstream ss;
    if (opmn.empty()) {
        // list a table of all ops
        // build header
        ss << std::setw(8) << std::left << "Mnemonic" << "  " << "Op" << "\n";
        for (auto &op : ops) {
            ss << std::setw(8) << std::left <<
                op.menmonic() << "  " << op.name() << "\n";
        }
    } else {
        bool prefixSearch = opmn[opmn.size() - 1] == '*';
        bool foundOp = false;
        for (auto &op : ops) {
            std::string mn = op.menmonic();
            if (opmn == mn ||
                (prefixSearch &&
                    opmn.substr(0, opmn.size() - 1) == mn.substr(0, opmn.size() - 1)))
            {
                foundOp = true;
                ss << "Mnemonic: " << mn << "\n";
                ss << "Name:     " << op.name() << "\n";
                ss << op.description() << "\n\n";
            }
        }
        hasError = !foundOp;
        if (!foundOp) {
            fatalExitWithMessage("iga: %s: mnemonic not found for platform",
                opmn.c_str());
        }
    }
    writeText(opts, ss.str().c_str());
    return hasError;
}
