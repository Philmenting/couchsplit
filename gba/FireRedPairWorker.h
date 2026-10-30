// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <csignal>
class QString;

// Explicit development opt-in only. The caller owns the process signal handler
// and the default mGBA logger. This path never polls physical controllers.
int runFireRedPairWorker(const QString& rom, volatile std::sig_atomic_t& quitRequested);
