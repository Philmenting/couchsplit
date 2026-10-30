// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdexcept>

namespace coop::firered {
// Shared worker/launcher process contract. Only this exact, expected boundary
// is given a special user-facing explanation; crashes and other errors retain
// the generic diagnostics and must never be mistaken for a capacity rejection.
inline constexpr int ExpShareCapacityExitCode = 71;
inline constexpr int EnigmaBerryExitCode = 72;
struct ExpShareCapacityError : std::runtime_error {
    ExpShareCapacityError() : std::runtime_error(
        "Too many FireRed EXP Share holders for the experimental shared battle") {}
};
struct EnigmaBerryError : std::runtime_error {
    EnigmaBerryError() : std::runtime_error(
        "FireRed Enigma Berry effects are not supported in a shared battle") {}
};
}
