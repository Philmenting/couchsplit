// SPDX-License-Identifier: GPL-3.0-or-later
// ROM-free tests: authored binary records, no commercial saves or game assets.
#include "../gba/FireRedPokemon.h"
#include <iostream>
#include <string>

using namespace coop::firered;
namespace {
unsigned checks = 0;
void check(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
template<class F> void rejects(F function, const char* message) {
    bool rejected = false;
    try { function(); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected, message);
}
PartyMon mon(unsigned personality, unsigned ot = 0x12345678) {
    PartyMon result;
    monPut32(result.bytes.data(), personality);
    monPut32(result.bytes.data() + 4, ot);
    result.bytes[8] = 0xBB; result.bytes[9] = 0xFF;
    result.bytes[18] = 5; result.bytes[19] = 2;
    result.bytes[20] = 0xBC; result.bytes[21] = 0xFF;
    result.bytes[84] = 10; result.bytes[85] = 0xFF;
    monPut16(result.bytes.data() + 86, 35);
    monPut16(result.bytes.data() + 88, 42);
    for (unsigned offset = 90; offset < 100; offset += 2) monPut16(result.bytes.data() + offset, 20);
    PartyMon::Secure plain{};
    monPut16(plain.data(), 1); monPut32(plain.data() + 4, 1000);
    plain[9] = 70; monPut16(plain.data() + 12, 33);
    monPut16(plain.data() + 14, 45); plain[20] = 35; plain[21] = 40;
    monPut32(plain.data() + 40, 0x3FFFFFFF);
    result.setSecure(plain);
    return result;
}
Party party(unsigned count, unsigned start) {
    Party result; result.count = uint8_t(count);
    for (unsigned i = 0; i < count; ++i) result.mons[i] = mon(start + i);
    return result;
}
struct Bus {
    std::array<uint8_t, 0x40000> memory{};
    unsigned writes = 0;
    uint32_t last = 0;
    mCore core{};
    Bus() {
        core.board = this;
        core.busRead8 = [](mCore* c, uint32_t address) -> uint32_t {
            auto& bus = *static_cast<Bus*>(c->board);
            check(address >= 0x02000000 && address < 0x02040000, "Read outside EWRAM");
            return bus.memory[address - 0x02000000];
        };
        core.busWrite8 = [](mCore* c, uint32_t address, uint8_t value) {
            auto& bus = *static_cast<Bus*>(c->board);
            check(address == PlayerPartyCountAddress ||
                  (address >= PlayerPartyAddress && address < PlayerPartyAddress + PartyMonSize * PartyCapacity),
                  "Write outside owned party RAM");
            bus.memory[address - 0x02000000] = value;
            ++bus.writes; bus.last = address;
        };
    }
};
void formats() {
    // Independently enumerated physical orders for all PID residues.
    const std::array<std::string, 24> expected{{
        "GAEM", "GAME", "GEAM", "GEMA", "GMAE", "GMEA",
        "AGEM", "AGME", "AEGM", "AEMG", "AMGE", "AMEG",
        "EGAM", "EGMA", "EAGM", "EAMG", "EMGA", "EMAG",
        "MGAE", "MGEA", "MAGE", "MAEG", "MEGA", "MEAG"}};
    const std::string logical = "GAEM";
    for (unsigned pid = 0; pid < 24; ++pid) {
        auto value = mon(pid);
        value.validate();
        const auto plain = value.secure();
        for (unsigned physical = 0; physical < 4; ++physical) {
            const auto wanted = unsigned(logical.find(expected[pid][physical]));
            for (unsigned word = 0; word < 3; ++word) {
                const auto decrypted = monU32(value.bytes.data() + 32 + physical * 12 + word * 4)
                                       ^ pid ^ 0x12345678u;
                check(decrypted == monU32(plain.data() + wanted * 12 + word * 4), "Wrong encrypted shuffle");
            }
        }
        check(value.species() == 1 && value.move(0) == 33 && value.move(1) == 45, "Wrong logical Pokemon fields");
        check(value.experience() == 1000 && value.level() == 10 && value.hp() == 35, "Wrong party extension fields");
        const auto original = value;
        value.setPp(0, 17); value.setHp(0); value.setStatus(8); value.validate();
        check(value.pp(0) == 17 && value.hp() == 0 && value.status() == 8 && !value.viable(), "Wrong field update");
        value.setPp(0, 35); value.setHp(35); value.setStatus(0);
        check(value == original, "Field updates changed unrelated Pokemon bytes");
    }
    PartyMon{}.validate();
    PartyMon nativeEmpty;
    nativeEmpty.bytes[8] = nativeEmpty.bytes[20] = 0xFF;
    nativeEmpty.bytes[84] = 0xFF; nativeEmpty.bytes[99] = 0xA5;
    check(nativeEmpty.empty(), "Native unused party slot with sentinel bytes was rejected");
    nativeEmpty.validate();
    auto nativeParty = party(1, 24);
    nativeParty.mons[5] = nativeEmpty; nativeParty.validate();
    const auto nativeTransaction = combineParties(nativeParty, party(1, 48));
    const auto nativeRestored = restoreParties(nativeTransaction, nativeTransaction.combined);
    check(nativeRestored[0] == nativeParty, "Native unused party sentinel bytes were not preserved");
    nativeEmpty.bytes[19] = 2;
    check(!nativeEmpty.empty(), "A species flag was accepted as an empty Pokemon");
    auto corrupt = mon(24); corrupt.bytes[32] ^= 1;
    rejects([&] { corrupt.validate(); }, "Corrupt checksum accepted");
    corrupt = mon(24); corrupt.bytes[19] |= 1;
    rejects([&] { corrupt.validate(); }, "Bad Egg accepted");
    corrupt = mon(24); corrupt.bytes[19] |= 4;
    rejects([&] { corrupt.validate(); }, "Inconsistent Egg flag accepted");
    corrupt = mon(24); corrupt.bytes[84] = 101;
    rejects([&] { corrupt.validate(); }, "Invalid level accepted");
    corrupt = mon(24); monPut16(corrupt.bytes.data() + 86, 43);
    rejects([&] { corrupt.validate(); }, "HP beyond maximum accepted");
    auto valid = mon(24);
    rejects([&] { valid.setPp(4, 1); }, "Invalid PP slot accepted");
    rejects([&] { valid.setPp(2, 1); }, "PP for empty move accepted");
    rejects([&] { valid.setStatus(0x1000); }, "Invalid status accepted");
    rejects([&] { valid.setHp(43); }, "Invalid HP setter accepted");
    auto bad = party(2, 24); bad.mons[1] = bad.mons[0];
    rejects([&] { bad.validate(); }, "Ambiguous party identities accepted");
    bad = party(2, 24); bad.mons[0] = {};
    rejects([&] { bad.validate(); }, "Party gap accepted");
    bad = party(1, 24); bad.count = 0;
    rejects([&] { bad.validate(); }, "Stale party count accepted");
}
void transactions() {
    for (unsigned firstCount = 1; firstCount <= 6; ++firstCount) {
        for (unsigned secondCount = 1; secondCount <= 6; ++secondCount) {
            const auto first = party(firstCount, 240), second = party(secondCount, 480);
            const auto transaction = combineParties(first, second);
            check(transaction.combined.count == std::min(firstCount, 3u) + std::min(secondCount, 3u), "Unequal party lost viable members");
            check(transaction.owners[0] == 0 && transaction.owners[1] == 1, "Wrong team leads");
            const auto restored = restoreParties(transaction, transaction.combined);
            check(restored[0] == first && restored[1] == second, "Unchanged battle lost bench/original ordering");
        }
    }
    auto first = party(6, 240), second = party(1, 480);
    first.mons[0].setHp(0);
    auto egg = first.mons[2].secure();
    monPut32(egg.data() + 40, monU32(egg.data() + 40) | 0x40000000u);
    first.mons[2].bytes[19] |= 4; first.mons[2].setSecure(egg);
    const auto transaction = combineParties(first, second);
    check(transaction.combined.count == 4, "Unequal viable teams incorrectly truncated");
    check(transaction.originalSlots[0] == 1 && transaction.originalSlots[1] == 0 &&
          transaction.originalSlots[2] == 3 && transaction.originalSlots[3] == 4, "Fainted/Egg/bench mapping failed");
    check(transaction.owners[2] == 0 && transaction.owners[3] == 0, "Unequal ownership must not assume parity");
    auto result = transaction.combined;
    result.mons[0].setHp(12); result.mons[1].setStatus(8); result.mons[2].setPp(0, 5);
    auto growth = result.mons[3].secure(); monPut32(growth.data() + 4, 1800); growth[24] = 5; growth[36] = 0x21; growth[44] = 0x80;
    result.mons[3].setSecure(growth);
    std::swap(result.mons[0], result.mons[3]);
    const auto restored = restoreParties(transaction, result);
    check(restored[0].mons[1].hp() == 12 && restored[1].mons[0].status() == 8 &&
          restored[0].mons[3].pp(0) == 5 && restored[0].mons[4].experience() == 1800, "Result ownership failed after native reorder");
    for (unsigned slot : {0u, 2u, 5u})
        check(restored[0].mons[slot] == first.mons[slot], "Unselected/fainted/Egg bench changed");
    auto invalid = result; invalid.mons[0] = mon(720);
    rejects([&] { restoreParties(transaction, invalid); }, "Foreign Pokemon result accepted");
    invalid = result; invalid.mons[0] = invalid.mons[1];
    rejects([&] { restoreParties(transaction, invalid); }, "Duplicate result accepted");
    invalid = result; invalid.mons[0].bytes[32] ^= 1;
    rejects([&] { restoreParties(transaction, invalid); }, "Corrupt result accepted");
    invalid = result; auto modified = invalid.mons[0].secure(); modified[40] ^= 1; invalid.mons[0].setSecure(modified);
    rejects([&] { restoreParties(transaction, invalid); }, "Changed IV identity accepted");
    invalid = result; auto lower = invalid.mons[0].secure(); monPut32(lower.data() + 4, 500); invalid.mons[0].setSecure(lower);
    rejects([&] { restoreParties(transaction, invalid); }, "Experience decrease accepted");
    invalid = result; invalid.mons[4] = mon(720); invalid.count = 5;
    rejects([&] { restoreParties(transaction, invalid); }, "Unassigned captured Pokemon accepted");
    auto badTransaction = transaction; badTransaction.owners[3] = 1;
    rejects([&] { restoreParties(badTransaction, result); }, "Changed ownership map accepted");
    badTransaction = transaction; badTransaction.originalSlots[5] = 0;
    rejects([&] { restoreParties(badTransaction, result); }, "Unused mapping accepted");
    auto duplicate = second; duplicate.mons[0] = first.mons[5];
    rejects([&] { combineParties(first, duplicate); }, "Cross-player duplicate identity accepted");
    auto fainted = party(1, 720); fainted.mons[0].setHp(0);
    rejects([&] { combineParties(first, fainted); }, "Team without viable lead accepted");
    rejects([&] { combineParties(first, second, {{{1, 1}, {0}}}); }, "Duplicate explicit selection accepted");
    const auto explicitTeam = combineParties(first, second, {{{5, 1}, {0}}});
    check(explicitTeam.originalSlots[0] == 5 && explicitTeam.originalSlots[2] == 1, "Explicit team selection ignored");
    check(restoreParties(explicitTeam, explicitTeam.combined)[0] == first, "Explicit selection lost original party");
}
void experienceShareSelection() {
    auto first = party(6, 240), second = party(6, 480);
    auto giveShare = [](PartyMon& mon) {
        auto secure = mon.secure(); monPut16(secure.data() + 2, 182); mon.setSecure(secure);
    };
    // Both players' sixth Pokemon were previously left out of the native
    // working set, silently changing the split and losing their own award.
    giveShare(first.mons[5]); giveShare(second.mons[5]);
    auto transaction = combineParties(first, second);
    check(transaction.originalSlots[0] == 0 && transaction.originalSlots[1] == 0,
          "EXP Share selection changed a trainer's normal lead");
    for (unsigned owner = 0; owner < 2; ++owner) {
        bool loaded = false;
        for (unsigned slot = 0; slot < transaction.combined.count; ++slot)
            loaded |= transaction.owners[slot] == int(owner) && transaction.originalSlots[slot] == 5;
        check(loaded, "Later EXP Share holder was not loaded for the native split");
    }
    check(restoreParties(transaction, transaction.combined) == std::array<Party, 2>{{first, second}},
          "EXP Share selection changed original ownership");
    // A fainted holder is ignored by the native award and must not occupy a
    // working slot. A living lead holding the item needs no additional slot.
    giveShare(first.mons[0]); giveShare(first.mons[4]); first.mons[4].setHp(0);
    transaction = combineParties(first, second);
    check(transaction.originalSlots[0] == 0 && transaction.combined.mons[0].heldItem() == 182,
          "EXP Share lead was selected incorrectly");
    giveShare(first.mons[3]);
    transaction = combineParties(first, second);
    bool thirdLoaded = false;
    for (unsigned slot = 0; slot < transaction.combined.count; ++slot)
        thirdLoaded |= transaction.owners[slot] == 0 && transaction.originalSlots[slot] == 3;
    check(thirdLoaded, "Second eligible reserve holder was omitted");
    giveShare(first.mons[2]);
    transaction = combineParties(first, second);
    unsigned firstLoaded = 0, secondLoaded = 0;
    for (unsigned slot = 0; slot < transaction.combined.count; ++slot) {
        firstLoaded += transaction.owners[slot] == 0;
        secondLoaded += transaction.owners[slot] == 1;
    }
    check(firstLoaded == 4 && secondLoaded == 2 &&
          restoreParties(transaction, transaction.combined) == std::array<Party, 2>{{first, second}},
          "Unequal native working set omitted an eligible EXP Share holder or changed ownership");
    giveShare(first.mons[1]);
    const auto before = first;
    bool capacityError = false;
    try { combineParties(first, second); } catch (const ExpShareCapacityError&) { capacityError = true; }
    check(capacityError, "Oversubscribed EXP Share set did not report the dedicated capacity error");
    check(first == before, "Rejected EXP Share selection modified the original party");
}
void enigmaBerrySelection() {
    for (unsigned owner=0; owner<2; ++owner) for (unsigned slot : {0u, 5u}) {
        auto first=party(6, 240), second=party(6, 480);
        auto& mon=(owner ? second : first).mons[slot];
        auto secure=mon.secure();monPut16(secure.data()+2,EnigmaBerryItem);mon.setSecure(secure);
        if(slot==5) mon.setHp(0); // Even a fainted reserve may be revived in battle.
        const auto before=std::array<Party,2>{{first,second}};
        bool automatic=false, explicitSelection=false;
        try { combineParties(first,second); } catch(const EnigmaBerryError&) { automatic=true; }
        try { combineParties(first,second,{{{0},{0}}}); } catch(const EnigmaBerryError&) { explicitSelection=true; }
        check(automatic && explicitSelection && before==std::array<Party,2>{{first,second}},
              "Enigma Berry was accepted or rejection modified an owner's party");
    }
}
bool sameTransaction(const BattleParty& a, const BattleParty& b) {
    return a.originals == b.originals && a.combined == b.combined && a.owners == b.owners && a.originalSlots == b.originalSlots;
}
void benchRotation() {
    const auto first = party(6, 240), second = party(6, 480);
    auto transaction = combineParties(first, second);
    auto current = transaction.combined;
    const std::array<unsigned, 6> excluded{{0, 1, 6, 0xFF, 6, 0xFF}};
    const auto initialTransaction = transaction;
    check(!refillBattleBench(transaction, current, 0, excluded), "Rotation ignored a living native bench member");
    check(sameTransaction(transaction, initialTransaction), "No-op refill changed its journal");
    auto expected = transaction.originals;
    std::vector<MonIdentity> seen;
    auto recordFaint = [&](unsigned slot) {
        const auto owner = unsigned(transaction.owners[slot]);
        const auto original = unsigned(transaction.originalSlots[slot]);
        auto& value = current.mons[slot];
        value.setHp(0); value.setStatus(8); value.setPp(0, 10);
        auto plain = value.secure(); monPut32(plain.data() + 4, 1100 + original); plain[24] = uint8_t(original + 1);
        value.setSecure(plain);
        expected[owner].mons[original] = value;
        if (std::find(seen.begin(), seen.end(), value.identity()) == seen.end()) seen.push_back(value.identity());
    };
    for (unsigned slot = 0; slot < current.count; ++slot) recordFaint(slot);
    // Reorder same-owner bench records as a native menu can do.
    std::swap(current.mons[2], current.mons[4]);
    for (unsigned owner = 0; owner < 2; ++owner) {
        for (unsigned replacement = 3; replacement < 6; ++replacement) {
            const auto before = current;
            const auto updated = refillBattleBench(transaction, current, owner, excluded);
            check(bool(updated), "Exhausted native working set lost a healthy original bench member");
            current = *updated; current.validate();
            unsigned changed = 0, destination = 6;
            for (unsigned slot = 0; slot < current.count; ++slot) {
                if (!(current.mons[slot] == before.mons[slot])) { ++changed; destination = slot; }
                if (slot == 0 || slot == 1) check(current.mons[slot] == before.mons[slot], "Rotation replaced an active Pokemon");
            }
            check(changed == 1 && destination < 6, "Rotation changed multiple native slots");
            check(transaction.owners[destination] == int(owner) && transaction.originalSlots[destination] == int(replacement),
                  "Rotation lost its owner/original mapping");
            for (unsigned left = 0; left < current.count; ++left)
                for (unsigned right = left + 1; right < current.count; ++right)
                    check(current.mons[left].identity() != current.mons[right].identity(), "Rotation duplicated a native identity");
            check(restoreParties(transaction, current) == expected, "Rebased working set lost retired fainted Pokemon data");
            for (unsigned untouched = replacement + 1; untouched < 6; ++untouched)
                check(transaction.originals[owner].mons[untouched] == (owner == 0 ? first : second).mons[untouched],
                      "Rotation modified an unused healthy original bench member");
            recordFaint(destination);
        }
        const auto beforeNoop = transaction;
        check(!refillBattleBench(transaction, current, owner, excluded), "Rotation invented a seventh owned Pokemon");
        check(sameTransaction(transaction, beforeNoop), "Exhausted no-op altered journal");
    }
    check(seen.size() == 12, "All twelve original Pokemon did not participate in rotations");
    check(restoreParties(transaction, current) == expected, "Final restoration lost HP/status/PP/XP/EV results across twelve Pokemon");
    auto blocked = combineParties(first, second);
    auto blockedCurrent = blocked.combined;
    for (unsigned slot = 0; slot < blockedCurrent.count; ++slot) blockedCurrent.mons[slot].setHp(0);
    const auto beforeBlocked = blocked;
    check(!refillBattleBench(blocked, blockedCurrent, 0, std::vector<unsigned>{0, 1, 2, 3, 4, 5}),
          "Rotation ignored active/queued slot exclusions");
    check(sameTransaction(blocked, beforeBlocked), "Blocked refill changed journal");
    auto corrupt = blockedCurrent; corrupt.mons[3].bytes[32] ^= 1;
    rejects([&] { refillBattleBench(blocked, corrupt, 0, excluded); }, "Unrelated corrupt native result accepted for rotation");
    check(sameTransaction(blocked, beforeBlocked), "Rejected corrupt refill mutated journal");
    rejects([&] { refillBattleBench(blocked, blockedCurrent, 2, excluded); }, "Invalid refill owner accepted");
    rejects([&] { refillBattleBench(blocked, blockedCurrent, 0, std::vector<unsigned>{7}); }, "Invalid excluded native index accepted");
    check(sameTransaction(blocked, beforeBlocked), "Invalid refill request mutated journal");
}
void memory() {
    Bus bus;
    const auto original = party(6, 240);
    writeParty(&bus.core, original);
    check(bus.writes == 601 && bus.last == PlayerPartyCountAddress, "Party count was not committed last");
    check(readParty(&bus.core) == original, "Memory party snapshot differs");
    const auto bytes = bus.memory;
    const auto writes = bus.writes;
    auto invalid = original; invalid.mons[4].bytes[33] ^= 1;
    rejects([&] { writeParty(&bus.core, invalid); }, "Invalid party write accepted");
    check(bytes == bus.memory && writes == bus.writes, "Invalid party partially committed");
    rejects([&] { writeParty(nullptr, original); }, "Null write core accepted");
    rejects([&] { readParty(nullptr); }, "Null read core accepted");
    writeParty(&bus.core, Party{});
    check(readParty(&bus.core) == Party{}, "Empty intro party rejected");
}
}
int main() {
    try {
        formats(); transactions(); experienceShareSelection(); enigmaBerrySelection(); benchRotation(); memory();
        std::cout << "FireRed Pokemon ownership: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
