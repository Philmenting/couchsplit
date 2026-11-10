// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedVersion.h"
#include "FireRedAddressData.h"
#include <mgba/core/core.h>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace coop::firered {

namespace profile_detail {
struct Binding { const mCore* core; RomVersion version; };
// A paired worker advances both cores and their hooks on its owning thread.
// Never infer a writable ABI from the cartridge's language/header bytes.
inline thread_local std::vector<Binding> bindings;
}

// The caller must fingerprint the complete ROM before binding/using its ABI.
// Keep this scope alive until all coordinators/hooks have been destroyed.
class RomProfileScope {
    const mCore* core;
public:
    RomProfileScope(const mCore* target, RomVersion version):core(target) {
        if (!core || (version != RomVersion::German && version != RomVersion::English10 &&
                      version != RomVersion::English11))
            throw std::runtime_error("Invalid FireRed ROM profile binding");
        for (const auto& binding:profile_detail::bindings)
            if (binding.core == core) throw std::runtime_error("FireRed core already has a ROM profile");
        profile_detail::bindings.push_back({core,version});
    }
    ~RomProfileScope() {
        auto& bindings=profile_detail::bindings;
        const auto entry=std::find_if(bindings.begin(),bindings.end(),
            [this](const auto& binding){return binding.core==core;});
        if (entry!=bindings.end()) bindings.erase(entry);
    }
    RomProfileScope(const RomProfileScope&)=delete;
    RomProfileScope& operator=(const RomProfileScope&)=delete;
    RomProfileScope(RomProfileScope&&)=delete;
    RomProfileScope& operator=(RomProfileScope&&)=delete;
};

inline RomVersion romVersion(const mCore* core) {
    for (const auto& binding:profile_detail::bindings)
        if (binding.core==core) return binding.version;
    // Preserve the existing German-only component harnesses and API. All
    // production paired cores bind an exact fingerprint explicitly.
    return RomVersion::German;
}

// Resolve ONLY a reviewed, fixed ABI location. Dynamic native pointers, RAM
// values, encrypted Pokemon payloads and scan ranges must never pass here.
inline uint32_t abiAddress(const mCore* core,uint32_t canonical) {
    const auto version=romVersion(core);
    if (version==RomVersion::German) return canonical;
    for (const auto& address:profile_data::addresses)
        if (address.german==canonical)
            return version==RomVersion::English10 ? address.english10 : address.english11;
    throw std::runtime_error("Unreviewed FireRed ABI location");
}

// Used only for exact native caller (LR) guards, never general bus values.
// An unknown English caller cannot accidentally match a German address.
inline uint32_t canonicalCodeAddress(const mCore* core,uint32_t native) {
    const auto version=romVersion(core);
    if (version==RomVersion::German) return native;
    for (const auto& address:profile_data::addresses)
        if (address.german>=0x08000000 && native==
            (version==RomVersion::English10 ? address.english10 : address.english11))
            return address.german;
    return 0;
}

inline std::array<uint16_t,4> hookPreimage(const mCore* core,uint32_t canonical,
                                         const std::array<uint16_t,4>& german) {
    const auto version=romVersion(core);
    if (version==RomVersion::German) return german;
    for (const auto& hook:profile_data::hooks)
        if (hook.german==canonical)
            return version==RomVersion::English10 ? hook.english10 : hook.english11;
    throw std::runtime_error("Unreviewed FireRed hook preimage");
}
} // namespace coop::firered
