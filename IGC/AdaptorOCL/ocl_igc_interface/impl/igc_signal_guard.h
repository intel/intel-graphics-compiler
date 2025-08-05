/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGC_SIGNAL_GUARD_H
#define IGC_SIGNAL_GUARD_H

#include <csignal>

namespace IGC::detail {
class SignalGuard {
public:
  SignalGuard(int Signal, void (*Handler)(int, siginfo_t *, void *)) : Signal(Signal) {
    sigaction(Signal, nullptr, &SAOld);
    if (SAOld.sa_handler == SIG_DFL) {
      struct sigaction SA;
      sigemptyset(&SA.sa_mask);
      SA.sa_sigaction = Handler;
      SA.sa_flags = 0;
      sigaction(Signal, &SA, nullptr);
    }
  }

  ~SignalGuard() {
    if (SAOld.sa_handler == SIG_DFL)
      sigaction(Signal, &SAOld, nullptr);
  }

  SignalGuard(const SignalGuard &) = delete;
  SignalGuard &operator=(const SignalGuard &) = delete;

private:
  const int Signal;
  struct sigaction SAOld;
}; // class SignalGuard
} // namespace IGC::detail

#define SET_SIG_HANDLER(SIG) ::IGC::detail::SignalGuard SG##SIG(SIG, signalHandler);

#define REMOVE_SIG_HANDLER(SIG) (void)SG##SIG;

#endif // IGC_SIGNAL_GUARD_H
