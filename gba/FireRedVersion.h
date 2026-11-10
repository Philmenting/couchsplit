// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <string_view>

namespace coop::firered {

enum class RomVersion { German, English10, English11 };
inline constexpr std::string_view GermanSha256 =
    "eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507";
inline constexpr std::string_view English10Sha256 =
    "3d0c79f1627022e18765766f6cb5ea067f6b5bf7dca115552189ad65a5c3a8ac";
inline constexpr std::string_view English11Sha256 =
    "729041b940afe031302d630fdbe57c0c145f3f7b6d9b8eca5e98678d0ca4d059";

// Header language/revision bytes do not authorize native memory operations.
// Every supported cartridge has an independently reviewed ABI and exact hash.
inline std::optional<RomVersion> fireRedVersion(std::string_view sha256) {
    if (sha256 == GermanSha256) return RomVersion::German;
    if (sha256 == English10Sha256) return RomVersion::English10;
    if (sha256 == English11Sha256) return RomVersion::English11;
    return std::nullopt;
}
} // namespace coop::firered
