#include <llvm/Support/Errc.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FileSystem.h>

#include <algorithm>
#include <cassert>
#include <fstream>

#include "InputArgsWrapper.h"

namespace {

using FEInputArgs = IGC::AdaptorCM::Frontend::IInputArgs;
template <class T> using SeqT = FEInputArgs::SeqT<T>;
using StrT = FEInputArgs::StrT;
using SeqStrT = SeqT<StrT>;

template<typename InputIt, typename OutputIt>
bool loadExtraFiles(InputIt ListStart, InputIt ListEnd, OutputIt ExtraFiles) {
  using FileT = typename OutputIt::container_type::value_type;
  std::transform(ListStart, ListEnd, ExtraFiles,
    [](const StrT &path) {
      std::ifstream file(path);
      if (!file.is_open()) {
        return FileT{"", ""};
      }
      std::string Src{std::istreambuf_iterator<char>(file),
                      std::istreambuf_iterator<char>()};
      return FileT{path, Src};
    });
  return true;
}

} // end anonymous namespace

namespace IGC {
namespace AdaptorCM {
namespace Frontend {

InputArgsWrapper::InputArgsWrapper(const FEIInput &In, ErrorType &Error)
  : Src(In.getSrc()) {

  Error.clear();

  const auto &Opts = In.getCompOpts();
  CompOpts.insert(CompOpts.end(), Opts.begin(), Opts.end());

  processSupportDirectories(In, Error);
}

void InputArgsWrapper::processSupportDirectories(const FEIInput &In, ErrorType &Error) {

  if (!Error.empty())
    return;

// TODO: enable this for Windows
#if __linux__
  // process files
  std::error_code EC;
  SeqStrT IncludeFiles;
  // TODO: this code is expected to load all "support" files (cm.h and such).
  // The way how/from where these files are loaded should be an internal logic
  // of CMAdaptor
  for (const auto &Dir: In.getSupportDirs()) {
    CompOpts.emplace_back("-isystem");
    CompOpts.emplace_back(Dir);
    bool IsEmpty = true;

    using dir_iterator = llvm::sys::fs::recursive_directory_iterator;
    dir_iterator i(Dir, EC);
    if (i == dir_iterator() ||
        i->type() == llvm::sys::fs::file_type::status_error ||
        i->type() == llvm::sys::fs::file_type::file_not_found) {
      Error.append("Could not open <").append(Dir).append("> as support dir\n");
      return;
    }
    for (dir_iterator e; i!=e; i.increment(EC)) {
      if (EC) {
        Error.append("an error has occured during traversal of support dir\n");
        return;
      }
      // TODO: revise this condition
      if (i->type() == llvm::sys::fs::file_type::regular_file &&
          llvm::StringRef(i->path()).endswith(".h")) {
        IncludeFiles.push_back(i->path());
        IsEmpty = false;
      }
    }
    if (IsEmpty) {
      // TODO: design a proper warning-reporting mechanism
      llvm::errs() << "Warning: support directory <" << Dir << "> is empty\n";
    }
  }
  if (!loadExtraFiles(IncludeFiles.begin(),
                      IncludeFiles.end(),
                      std::back_inserter(ExtraFiles))) {
    Error.append("could not load extra files\n");
    return;
  }
  assert(In.getExtraFiles().size() == 0 && "extra files are not supported (TODO)");
#endif // __linux_
}


} // AdaptorCM
} // Frontend
} // IGC
