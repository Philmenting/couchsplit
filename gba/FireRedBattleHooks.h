// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedEncounter.h"
#include "FireRedPartyMenu.h"
#include "FireRedReserves.h"
#include <mgba/core/core.h>
#include <mgba/internal/arm/arm.h>
#include <array>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace coop::firered {

// Exact German FireRed only; callers verify the complete ROM fingerprint.
// Install/detach while paused and keep this object alive through every CPU run.
// Pinned mGBA's raw patch API creates an in-memory ROM copy; no ROM file writes.
// CPU snapshots do not own these patches/callbacks: detach on rollback and before
// core destruction. Only check() may propagate predicate errors to the caller.
class BattleHooks {
public:
    using SwitchPredicate = std::function<bool(unsigned battler, unsigned excludedOne, unsigned excludedTwo)>;
    // Compare the native OT against the actual holder of this combined-party
    // slot, preserving legitimate traded-mon status. Name is a native RAM ptr.
    using OtherTrainerPredicate = std::function<bool(unsigned slot, uint32_t otId, uint32_t otName)>;
    using BadgePredicate = std::function<bool(unsigned battler, unsigned flag)>;
    using CaptureFullPredicate = std::function<bool()>;
    using BallThrowObserver = std::function<void(unsigned battler, unsigned item)>;
    using TeamHealthObserver = std::function<void()>;
    using CapturedMonObserver = std::function<void(uint32_t monPointer)>;
    using EvolutionObserver = std::function<void(unsigned nativePartySlot)>;
    using GameStatisticHandler = std::function<bool(unsigned statistic)>;
    using PartyMenuOpen = std::function<void(unsigned battler)>;
    using PartyMenuReturn = std::function<void()>;
    using PartyMenuReply = std::function<std::optional<unsigned>(unsigned selected)>;
    using PartyMenuSelection = std::function<bool(unsigned selected)>;
    using MoneyAnnouncement = std::function<uint32_t(bool payDay, uint32_t nativeAmount)>;
    using BallTextName = std::function<std::optional<std::array<uint8_t,7>>(unsigned battler)>;
    using SingleWild = std::function<bool()>;
    using SingleTrainer = std::function<bool()>;
    using ExperienceCommand = std::function<bool()>;
    using EvolutionGate = std::function<void()>;
    using EvolutionQueue = std::function<bool(bool ending)>;
    using OpponentReset = std::function<void(unsigned battler)>;
    using EvolutionDex = std::function<std::optional<bool>(unsigned slot)>;
    using WishCast = std::function<void(unsigned battler,unsigned slot)>;
    using WishName = std::function<std::optional<WishJournal::Name>(unsigned battler,unsigned slot)>;
    using ForcedSwitch = std::function<std::optional<unsigned>(unsigned battler,unsigned original)>;
    // Exact native Wish success and B_BUFF1 expansion, and trainer force-out
    // count / random-choice boundaries. Enemy and wild force-outs are native.
    static constexpr uint32_t WishCastAddress=0x0802C3DE;
    static constexpr uint32_t WishTextAddress=0x080D7C6C;
    static constexpr uint32_t ForcedCountAddress=0x0802838C;
    static constexpr uint32_t ForcedChoiceAddress=0x080284A4;
    static constexpr uint32_t EvolutionDexAddress=0x0806E1C0;
    static constexpr uint32_t ExperienceCommandAddress=0x080219EC;
    static constexpr uint32_t EvolutionGateAddress=0x080159C6;
    static constexpr uint32_t EvolutionQueueAddress=0x08015A24;
    static constexpr uint32_t EvolutionQueueEndAddress=0x08015A9A;
    static constexpr uint32_t OpponentResetAddress=0x080173B8;
    static constexpr uint32_t HasNoMonsAddress = 0x08019B94;
    static constexpr uint32_t IsOtherTrainerAddress = 0x08044188;
    static constexpr uint32_t FlagGetAddress = 0x0806E634;
    static constexpr uint32_t CaptureFullAddress = 0x08040E6C;
    // HandleAction_UseItem, after the actual attacker and item are assigned,
    // immediately before selecting the native ball script. Queued menu choices
    // do not identify which throw actually ran.
    static constexpr uint32_t BallThrowAddress = 0x080163E8;
    static constexpr uint32_t CheckTeamsLostAddress = 0x0802237C;
    static constexpr uint32_t GiveMonAddress = 0x08040A14;
    static constexpr uint32_t EvolutionAddress = 0x080CDF94;
    static constexpr uint32_t IncrementGameStatAddress = 0x08054EB0;
    static constexpr uint32_t TrainerMoneyTextAddress = 0x08025984;
    static constexpr uint32_t PayDayTextAddress = 0x08028774;
    static constexpr uint32_t PartyMenuOpenAddress = 0x08032C50;
    static constexpr uint32_t PartyMenuReturnAddress = NativePartyMenuReshowAddress;
    static constexpr uint32_t PartyMenuReturn2Address = 0x080309EC;
    static constexpr uint32_t PartyMenuReplyAddress = NativeChosenMonReturnAddress;
    // TrySwitchInPokemon, after native eligibility checks and original-slot
    // translation, before selected ID, exit flag or party order are mutated.
    static constexpr uint32_t PartyMenuSelectionAddress = 0x08127C46;
    // TryRunFromBattle: the native double-battle check is evaluated after
    // trapping/guaranteed-escape checks and immediately before the speed roll.
    static constexpr uint32_t WildEscapeDoubleCheckAddress = 0x080167A8;
    // BattleStringExpandPlaceholders' native {B_PLAYER_NAME} case.
    static constexpr uint32_t PlayerNameTextAddress = 0x080D83E6;
    // A converted single encounter still uses the native double controller
    // layout for both humans. The following sites keep only its real opponent
    // active through the initial draw, trainer send-out and menu reshow. They
    // do not change the battle-type flags or genuine two-opponent battles.
    static constexpr uint32_t WildIntroSpriteAddress = 0x08013238;
    static constexpr uint32_t WildIntroTextAddress = 0x080D75AC;
    static constexpr uint32_t WildBallTargetAddress = 0x0802D3E8;
    static constexpr uint32_t TrainerIntroBattlerAddress = 0x0801313A;
    static constexpr uint32_t TrainerIntroTextAddress = 0x080D761C;
    static constexpr uint32_t BattlerSpritePresentAddress = 0x08075188;
    static constexpr uint32_t IsDoubleBattleAddress = 0x080751F4;
    static constexpr uint32_t ReshowLoadSpriteAddress = 0x08077A70;
    static constexpr uint32_t ReshowCreateSpriteAddress = 0x08077B9C;
    static constexpr uint32_t ReshowCreateHealthboxAddress = 0x08077F48;
private:
    struct Site {
        uint32_t address;
        uint16_t breakpoint;
        std::array<uint16_t,4> preimage;
        bool enabled;
    };
    mCore* core = nullptr;
    ARMCore* cpu = nullptr;
    void (*previous)(ARMCore*, int) = nullptr;
    SwitchPredicate predicate;
    OtherTrainerPredicate otherTrainer;
    BadgePredicate badge;
    CaptureFullPredicate captureFull;
    BallThrowObserver ballThrow;
    TeamHealthObserver teamHealth;
    CapturedMonObserver capturedMon;
    EvolutionObserver evolution;
    GameStatisticHandler gameStatistic;
    MoneyAnnouncement moneyAnnouncement;
    PartyMenuOpen partyMenuOpen;
    PartyMenuReturn partyMenuReturn;
    PartyMenuReply partyMenuReply;
    PartyMenuSelection partyMenuSelection;
    BallTextName ballTextName;
    SingleWild singleWild;
    SingleTrainer singleTrainer;
    ExperienceCommand experienceCommand;
    EvolutionGate evolutionGate;
    EvolutionQueue evolutionQueue;
    OpponentReset opponentReset;
    EvolutionDex evolutionDex;
    WishCast wishCast;
    WishName wishName;
    ForcedSwitch forcedSwitch;
    std::array<Site,38> sites;
    std::exception_ptr error;
    bool attached = false;
    inline static std::mutex registryMutex;
    inline static std::unordered_map<ARMCore*, BattleHooks*> registry;

    void replaceInstruction(const Site& site, uint16_t expected, uint16_t replacement) noexcept {
        core->rawWrite16(core, site.address, -1, replacement);
        // Refresh either paused Thumb prefetch slot, including detach before a
        // prefetched BKPT has executed. Do not replace an already queued fake BX.
        if (cpu->executionMode != MODE_THUMB) return;
        const uint32_t pc = uint32_t(cpu->gprs[ARM_PC]);
        for (unsigned i = 0; i < 2; ++i)
            if (pc - 2 + i * 2 == site.address && (cpu->prefetch[i] & 0xFFFFu) == expected)
                cpu->prefetch[i] = replacement;
    }
    static void fallback(ARMCore* active, int immediate, BattleHooks* self) noexcept {
        if (self && self->previous) self->previous(active, immediate);
        else if (active->irqh.hitIllegal) active->irqh.hitIllegal(active, 0xBE00u | (unsigned(immediate) & 255u));
        else ARMRaiseUndefined(active);
    }
    unsigned partySlot(uint32_t pointer) const {
        constexpr uint32_t base = 0x02024284;
        if (pointer < base || pointer >= base + 600 || (pointer - base) % 100)
            throw std::runtime_error("Invalid FireRed native traded-mon context");
        return (pointer - base) / 100;
    }
    std::optional<bool> otherTrainerResult() {
        const uint32_t lr = uint32_t(cpu->gprs[ARM_LR]);
        unsigned slot;
        if (lr == 0x0804417Bu) {
            // IsTradedMon retains its original Pokemon* in r4 while constructing
            // the OT-name stack buffer. This also covers EXP Share recipients.
            slot = partySlot(uint32_t(cpu->gprs[4]));
        } else if (lr == 0x0801D405u) {
            const unsigned battler = core->busRead8(core, 0x02023D6B);
            if (battler != 0 && battler != 2) return std::nullopt;
            slot = core->busRead16(core, 0x02023BCE + battler * 2);
            if (slot >= 6) throw std::runtime_error("Invalid FireRed obedience party context");
        } else return std::nullopt;
        const uint32_t name = uint32_t(cpu->gprs[1]);
        if (!((name >= 0x02000000 && name <= 0x0203FFF8) ||
              (name >= 0x03000000 && name <= 0x03007FF8)))
            throw std::runtime_error("Invalid FireRed native OT name buffer");
        return otherTrainer(slot, uint32_t(cpu->gprs[0]), name);
    }
    std::optional<bool> badgeResult() {
        const uint32_t lr = uint32_t(cpu->gprs[ARM_LR]);
        unsigned battler = 4, expectedFlag = 0;
        switch (lr) {
        case 0x0801D411: expectedFlag = 0x827; break; // obedience: eighth badge
        case 0x0801D41F: expectedFlag = 0x821; break;
        case 0x0801D42D: expectedFlag = 0x823; break;
        case 0x0801D43B: expectedFlag = 0x825; break;
        case 0x0803ED25: expectedFlag = 0x820; break; // damage: attacker attack
        case 0x0803ED8D: expectedFlag = 0x826; break; // damage: attacker sp.attack
        case 0x0803ED57: expectedFlag = 0x824; battler = unsigned(cpu->gprs[4]); break;
        case 0x0803EDC5: expectedFlag = 0x826; battler = unsigned(cpu->gprs[4]); break;
        case 0x08014D9F: expectedFlag = 0x822; battler = unsigned(cpu->gprs[9]); break;
        case 0x08014E87: expectedFlag = 0x822; battler = unsigned(cpu->gprs[10]); break;
        default: return std::nullopt;
        }
        if (lr >= 0x0801D411 && lr <= 0x0801D43B)
            battler = core->busRead8(core, 0x02023D6B);
        if (lr == 0x0803ED25 || lr == 0x0803ED8D) {
            const uint32_t sp = uint32_t(cpu->gprs[ARM_SP]);
            if (sp < 0x03000000 || sp > 0x03007FE8 || (sp & 3u))
                throw std::runtime_error("Invalid FireRed damage badge stack context");
            battler = core->busRead32(core, sp + 16);
        }
        const unsigned flag = unsigned(cpu->gprs[0]) & 0xFFFFu;
        if (flag != expectedFlag || battler >= 4)
            throw std::runtime_error("Unexpected FireRed badge call context");
        if (battler != 0 && battler != 2) return std::nullopt;
        return badge(battler, flag);
    }
    void suppressPhantomOpponent() {
        if (core->busRead16(core,0x02023BD0) != core->busRead16(core,0x02023BD4))
            throw std::runtime_error("Unexpected FireRed single-opponent battler slots");
        core->busWrite8(core,0x02023D70,core->busRead8(core,0x02023D70)|8u);
        constexpr uint32_t phantom=0x02023BE4u+3u*88u;
        core->busWrite8(core,phantom+0x20,0); // no switch-in ability
        core->busWrite8(core,phantom+0x28,0);core->busWrite8(core,phantom+0x29,0);
        core->busWrite8(core,phantom+0x2E,0);core->busWrite8(core,phantom+0x2F,0);
    }
    static void breakpoint(ARMCore* active, int immediate) noexcept {
        BattleHooks* self = nullptr;
        {
            std::lock_guard<std::mutex> guard(registryMutex);
            auto entry = registry.find(active);
            if (entry != registry.end()) self = entry->second;
        }
        const Site* site = nullptr;
        if (self && active->executionMode == MODE_THUMB)
            for (const auto& candidate : self->sites)
                if (candidate.enabled && immediate == int(candidate.breakpoint & 255u) &&
                    uint32_t(active->gprs[ARM_PC]) == candidate.address + 4) site = &candidate;
        if (!site) { fallback(active, immediate, self); return; }
        // Own-party switch menus temporarily use the single-layout flag. Their
        // return hook MUST run before ordinary double-battle flag filtering,
        // restoring both physical parties before the native reshow reads them.
        if(site->address==PartyMenuOpenAddress || site->address==PartyMenuReturnAddress ||
           site->address==PartyMenuReturn2Address || site->address==PartyMenuReplyAddress ||
           site->address==PartyMenuSelectionAddress) {
            uint16_t instruction=site->preimage[0];
            try {
                if(!self->error) {
                    if(site->address==PartyMenuSelectionAddress) {
                        if(!self->partyMenuSelection(unsigned(active->gprs[0])&255u))
                            instruction=0xE048; // B 0x08127CDA: existing FALSE epilogue, same stack.
                    } else if(site->address==PartyMenuOpenAddress)
                        self->partyMenuOpen(self->core->busRead8(self->core,0x02023BC4));
                    else if(site->address==PartyMenuReplyAddress) {
                        if(const auto selected=self->partyMenuReply(unsigned(active->gprs[1])&255u)) {
                            active->gprs[1]=int32_t(*selected);
                            active->gprs[2]=int32_t(NativePartyMenuOrderAddress);
                        }
                    } else self->partyMenuReturn();
                }
            } catch(...) {self->error=std::current_exception();}
            ARMRunFake(active,instruction);return;
        }
        const auto flags = self->core->busRead32(self->core, 0x02022B4C);
        if (!(flags & 1u) || !sharedEncounterFlags(flags)) { ARMRunFake(active, site->preimage[0]); return; }
        if(site->address==WishCastAddress || site->address==WishTextAddress ||
           site->address==ForcedCountAddress || site->address==ForcedChoiceAddress) {
            uint16_t instruction=site->preimage[0];
            try {
                if(!self->error && site->address==WishCastAddress) {
                    const unsigned battler=self->core->busRead8(self->core,0x02023D6B);
                    if(battler==0 || battler==2) {
                        const auto slot=self->core->busRead8(self->core,0x02023F44+battler);
                        if(self->core->busRead8(self->core,0x02023F40+battler)!=2 ||
                           self->core->busRead16(self->core,0x02023BCE + battler*2)!=slot)
                            throw std::runtime_error("Unexpected FireRed Wish success context");
                        self->wishCast(battler,slot);
                    }
                } else if(!self->error && site->address==WishTextAddress &&
                          uint32_t(active->gprs[9])==0x083FBE79u) {
                    // Exact DE Wish-came-true string's B_BUFF1 placeholder.
                    // Keep the structured-buffer path: raw Buff1 would treat
                    // status-looking nicknames as native status messages.
                    constexpr uint32_t buffer=0x02022AB8, destination=0x02021CD0;
                    if(uint32_t(active->gprs[0])!=buffer || uint32_t(active->gprs[4])!=destination ||
                       self->core->busRead8(self->core,buffer)!=0xFD ||
                       self->core->busRead8(self->core,buffer+1)!=4 ||
                       self->core->busRead8(self->core,buffer+4)!=0xFF)
                        throw std::runtime_error("Unexpected FireRed Wish text context");
                    const auto battler=self->core->busRead8(self->core,buffer+2);
                    if(battler==0 || battler==2) {
                        if(const auto name=self->wishName(battler,self->core->busRead8(self->core,buffer+3))) {
                            for(unsigned i=0;i<name->size();++i)self->core->busWrite8(self->core,destination+i,(*name)[i]);
                            instruction=0xE001; // skip expansion call; retain native copy from gStringVar1.
                        }
                    }
                } else if(!self->error && (site->address==ForcedCountAddress || site->address==ForcedChoiceAddress) && (flags&8u)) {
                    const auto battler=self->core->busRead8(self->core,0x02023D6C);
                    if(battler==0 || battler==2) {
                        if(site->address==ForcedCountAddress)
                            active->gprs[7]=self->forcedSwitch(battler,6)?3:0;
                        else {
                            const auto selected=self->forcedSwitch(battler,unsigned(active->gprs[5]));
                            if(selected) {
                                if(*selected>=6)throw std::runtime_error("Invalid FireRed forced native slot");
                                active->gprs[5]=int32_t(*selected);
                                instruction=0xE04F; // B 0x08028546: native queue/order commit.
                            } else instruction=0xE7F5; // B 0x08028492: next native Random draw.
                        }
                    }
                }
            } catch(...) {self->error=std::current_exception();}
            ARMRunFake(active,instruction);return;
        }
        if(site->address==EvolutionDexAddress) {
            std::optional<bool> enabled;
            try {
                if(!self->error && uint32_t(active->gprs[ARM_LR])==0x080CEAA9u)
                    enabled=self->evolutionDex(self->partySlot(uint32_t(active->gprs[9])));
            } catch(...) {self->error=std::current_exception();}
            if(enabled)active->gprs[0]=*enabled?1:0;
            ARMRunFake(active,enabled?0x4770:site->preimage[0]);return;
        }
        if(site->address==ExperienceCommandAddress || site->address==EvolutionGateAddress ||
           site->address==EvolutionQueueAddress || site->address==EvolutionQueueEndAddress ||
           site->address==OpponentResetAddress) {
            uint16_t instruction=site->preimage[0];
            try {
                if(!self->error) {
                    if(site->address==ExperienceCommandAddress) {
                        if(self->experienceCommand())instruction=0x4770; // function entry: BX LR
                    } else if(site->address==EvolutionGateAddress) self->evolutionGate();
                    else if(site->address==OpponentResetAddress) self->opponentReset(unsigned(active->gprs[0])&255u);
                    else {
                        const bool ending=site->address==EvolutionQueueEndAddress;
                        if(self->evolutionQueue(ending)) {
                            if(!ending || uint32_t(active->gprs[3])!=0x03004F24u)
                                throw std::runtime_error("Unexpected FireRed evolution loop context");
                            instruction=0xE7C9; // within the existing stack frame: B 0x08015A30
                        }
                    }
                }
            } catch(...) {self->error=std::current_exception();}
            ARMRunFake(active,instruction);return;
        }
        if (site->address == TrainerIntroBattlerAddress || site->address == TrainerIntroTextAddress ||
            site->address == BattlerSpritePresentAddress || site->address == IsDoubleBattleAddress ||
            site->address == ReshowLoadSpriteAddress || site->address == ReshowCreateSpriteAddress ||
            site->address == ReshowCreateHealthboxAddress) {
            std::optional<unsigned> returnValue;
            try {
                if (!self->error) {
                    const bool trainer=(flags & 8u) && self->singleTrainer && self->singleTrainer();
                    const bool wild=!(flags & 8u) && self->singleWild && self->singleWild();
                    const bool absent=self->core->busRead8(self->core,0x02023D70)&8u;
                    if (trainer && site->address == TrainerIntroBattlerAddress &&
                        self->core->busRead8(self->core,0x02023BC4)==3) self->suppressPhantomOpponent();
                    else if (trainer && site->address == TrainerIntroTextAddress) {
                        if (uint32_t(active->gprs[1])!=flags || !absent)
                            throw std::runtime_error("Unexpected FireRed single-trainer intro text");
                        active->gprs[1]=int32_t(flags & ~1u);
                    } else if ((trainer || wild) && absent && site->address == BattlerSpritePresentAddress &&
                               (unsigned(active->gprs[0]) & 255u)==3) returnValue=0;
                    else if ((trainer || wild) && absent && (unsigned(active->gprs[0]) & 255u)==3 &&
                             (site->address==ReshowLoadSpriteAddress || site->address==ReshowCreateSpriteAddress ||
                              site->address==ReshowCreateHealthboxAddress)) {
                        // Reshow runs after either bag or party menus. It does
                        // not check absent flags and otherwise recreates the
                        // phantom's sprite and healthbox from the shared slot.
                        returnValue=site->address==ReshowLoadSpriteAddress ? 1u : 0u;
                    } else if ((trainer || wild) && absent && site->address == IsDoubleBattleAddress) {
                        const auto lr=uint32_t(active->gprs[ARM_LR]);
                        if (lr==0x0807794Bu) returnValue=0; // reshow's second shadow
                        // Only the opponent's native intro state machine sees a
                        // single layout; the human side remains a double.
                        else if (trainer && (lr==0x08038CEDu || lr==0x08035B39u || lr==0x08035B43u || lr==0x08035D9Bu)) {
                            if (self->core->busRead8(self->core,0x02023BC4)!=1)
                                throw std::runtime_error("Unexpected FireRed opponent intro controller");
                            returnValue=0;
                        }
                    }
                }
            } catch (...) { self->error=std::current_exception(); }
            if (returnValue) active->gprs[0]=int32_t(*returnValue);
            ARMRunFake(active,returnValue?0x4770:site->preimage[0]);
            return;
        }
        if (site->address == WildIntroSpriteAddress || site->address == WildIntroTextAddress ||
            site->address == WildBallTargetAddress) {
            bool skipSprite = false;
            try {
                if (!self->error && !(flags & 8u) && self->singleWild()) {
                    if (site->address == WildIntroSpriteAddress && self->core->busRead8(self->core,0x02023BC4)==3) {
                        self->suppressPhantomOpponent();
                        skipSprite = true;
                    } else if (site->address == WildIntroTextAddress) {
                        if (uint32_t(active->gprs[1]) != flags || !(self->core->busRead8(self->core,0x02023D70)&8u))
                            throw std::runtime_error("Unexpected FireRed single-wild intro text context");
                        active->gprs[1]=int32_t(flags & ~1u);
                    } else if (site->address == WildBallTargetAddress &&
                               self->core->busRead8(self->core,0x02023D6B)==2) {
                        if (uint32_t(active->gprs[0])!=3 || uint32_t(active->gprs[6])!=0x02023D6Cu ||
                            !(self->core->busRead8(self->core,0x02023D70)&8u))
                            throw std::runtime_error("Unexpected FireRed single-wild ball target");
                        active->gprs[0]=1;
                    }
                }
            } catch (...) { self->error = std::current_exception(); }
            // Skip the phantom's sprite emission AND its controller mark.
            ARMRunFake(active, skipSprite ? 0xE005 : site->preimage[0]);
            return;
        }
        if (site->address == PlayerNameTextAddress) {
            bool substituted = false;
            try {
                // Limit this to the exact German "player used item" string,
                // after a real ball action. The native 32-byte local text
                // buffer is scratch space here; never alter the save identity.
                if (!self->error && uint32_t(active->gprs[9]) == 0x083FD4D9u) {
                    const unsigned battler = self->core->busRead8(self->core, 0x02023D6B);
                    const unsigned item = self->core->busRead16(self->core, 0x02023D68);
                    if ((battler == 0 || battler == 2) && item >= 1 && item <= 12) {
                        if (const auto name = self->ballTextName(battler)) {
                            const uint32_t sp = uint32_t(active->gprs[ARM_SP]);
                            if (uint32_t(active->gprs[0]) != 0x03004F5Cu ||
                                sp < 0x03000000u || sp > 0x03007FE0u || (sp & 3u))
                                throw std::runtime_error("Unexpected FireRed item text context");
                            for (unsigned i = 0; i < name->size(); ++i)
                                self->core->busWrite8(self->core, sp + i, (*name)[i]);
                            self->core->busWrite8(self->core, sp + name->size(), 0xFF);
                            active->gprs[0] = int32_t(sp);
                            substituted = true;
                        }
                    }
                }
            } catch (...) { self->error = std::current_exception(); }
            // MOV r4,r0 substitutes the scratch string for the original
            // LDR r4,[r0] without changing condition flags.
            ARMRunFake(active, substituted ? 0x4604 : site->preimage[0]);
            return;
        }
        if (site->address == WildEscapeDoubleCheckAddress) {
            try {
                // The original game disallows the normal escape roll for a
                // double battle. Our wild doubles started as single encounters;
                // hide only the synthetic flag in this local register so the
                // original speed/RNG calculation runs. Do not change global
                // battle flags, trainer battles, or an opponent's own action.
                const unsigned battler = unsigned(active->gprs[4]);
                if (!(flags & 8u) && (battler == 0 || battler == 2)) {
                    if (uint32_t(active->gprs[3]) != flags)
                        throw std::runtime_error("Unexpected FireRed escape check context");
                    active->gprs[3] = int32_t(flags & ~1u);
                }
            } catch (...) { self->error = std::current_exception(); }
            ARMRunFake(active, site->preimage[0]);
            return;
        }
        if (site->address == TrainerMoneyTextAddress || site->address == PayDayTextAddress) {
            try {
                // r4 is the completed uncapped number, immediately before the
                // native text-buffer encoder. Preserve its original LDR and
                // every battle flag; only the displayed owner's amount changes.
                if (!self->error) active->gprs[4] = int32_t(self->moneyAnnouncement(
                    site->address == PayDayTextAddress, uint32_t(active->gprs[4])));
            } catch (...) { self->error = std::current_exception(); }
            ARMRunFake(active, site->preimage[0]);
            return;
        }
        if (site->address == BallThrowAddress) {
            try {
                if (!self->error) {
                    const unsigned battler = self->core->busRead8(self->core, 0x02023D6B);
                    const unsigned item = self->core->busRead16(self->core, 0x02023D68);
                    if ((battler != 0 && battler != 2) || item < 1 || item > 12)
                        throw std::runtime_error("Unexpected FireRed native ball action");
                    self->ballThrow(battler, item);
                }
            } catch (...) { self->error = std::current_exception(); }
            // This site is inside a native function, so continue its original
            // instruction even on error; the coordinator then rolls back.
            ARMRunFake(active, site->preimage[0]);
            return;
        }
        if (site->address == CheckTeamsLostAddress) {
            try {
                // Native Cmd_checkteamslost must see a refill before summing
                // the six working slots. Its early controller wait stays native.
                if (!self->error && self->core->busRead32(self->core, 0x02023BC8) == 0)
                    self->teamHealth();
            } catch (...) { self->error = std::current_exception(); }
            ARMRunFake(active, site->preimage[0]);
            return;
        }
        if (site->address == GiveMonAddress) {
            try {
                // Only the native caught-mon command is relevant. Field gifts,
                // trades and other callers retain their ordinary behavior.
                if (!self->error && uint32_t(active->gprs[ARM_LR]) == 0x0802D7C1u)
                    self->capturedMon(uint32_t(active->gprs[0]));
            } catch (...) { self->error=std::current_exception(); }
            ARMRunFake(active,site->preimage[0]);
            return;
        }
        if (site->address == EvolutionAddress) {
            try {
                // Field stones, trades and other callers retain native control.
                // This exact caller is TryEvolvePokemon after a won battle.
                if (!self->error && uint32_t(active->gprs[ARM_LR]) == 0x08015A77u &&
                    self->core->busRead8(self->core, 0x02023E8A) == 1) {
                    const unsigned slot = unsigned(active->gprs[3]) & 255u;
                    if (slot >= 6 || uint32_t(active->gprs[0]) != 0x02024284u + slot * 100u ||
                        (unsigned(active->gprs[2]) & 255u) != 0x81u)
                        throw std::runtime_error("Unexpected FireRed post-battle evolution context");
                    self->evolution(slot);
                }
            } catch (...) { self->error = std::current_exception(); }
            ARMRunFake(active, site->preimage[0]);
            return;
        }
        if (site->address == IncrementGameStatAddress) {
            bool handled = false;
            try {
                const unsigned statistic = unsigned(active->gprs[0]) & 255u;
                const auto lr = uint32_t(active->gprs[ARM_LR]);
                // Only the regular evolution task and battle-script action
                // counter callsites have an unambiguous paired-player owner.
                if (self->error) handled = true;
                else if ((statistic == 14 && lr == 0x080CEF45u) ||
                         ((statistic == 26 || statistic == 27) && lr == 0x08025B13u))
                    handled = self->gameStatistic(statistic);
            } catch (...) { self->error = std::current_exception(); handled = true; }
            // IncrementGameStat returns void. A handled action was incremented
            // with the real owner's key; skip the native leader increment.
            ARMRunFake(active, handled ? 0x4770 : site->preimage[0]);
            return;
        }
        std::optional<bool> result;
        try {
            if (self->error) result = site->address != FlagGetAddress;
            else if (site->address == HasNoMonsAddress) {
                const unsigned battler = unsigned(active->gprs[0]) & 255u;
                if (battler == 0 || battler == 2)
                    result = self->predicate(battler, unsigned(active->gprs[1]) & 255u, unsigned(active->gprs[2]) & 255u);
            } else if (site->address == IsOtherTrainerAddress) result = self->otherTrainerResult();
            else if (site->address == CaptureFullAddress) {
                // The Safari-action caller and all field callers stay native.
                if (uint32_t(active->gprs[ARM_LR]) == 0x080A1F13u) result = self->captureFull();
            } else result = self->badgeResult();
        } catch (...) {
            // Rollback is the coordinator's responsibility after this CPU run.
            self->error = std::current_exception();
            result = site->address != FlagGetAddress;
        }
        if (!result) { ARMRunFake(active, site->preimage[0]); return; }
        active->gprs[0] = *result ? 1 : 0;
        ARMRunFake(active, 0x4770); // authored BX LR; no native prologue ran
    }
public:
    BattleHooks(mCore* target, SwitchPredicate ownerHasNoMons,
                OtherTrainerPredicate ownerIsOtherTrainer = {}, BadgePredicate ownerHasBadge = {},
                CaptureFullPredicate ownerCaptureFull = {}, BallThrowObserver executedBallThrow = {},
                TeamHealthObserver beforeTeamHealthCheck = {}, CapturedMonObserver beforeCapturedMonGift = {},
                EvolutionObserver ownerEvolution = {}, GameStatisticHandler ownedGameStatistic = {},
                MoneyAnnouncement announcedMoney = {}, PartyMenuOpen beforePartyMenu = {},
                PartyMenuReturn beforePartyMenuReshow = {}, PartyMenuReply normalizePartyMenuReply = {},
                BallTextName ownerBallTextName = {}, SingleWild isSingleWild = {}, SingleTrainer isSingleTrainer = {},
                ExperienceCommand nativeExperience = {}, EvolutionGate beforeEvolution = {},
                EvolutionQueue nativeEvolutionQueue = {}, OpponentReset resetParticipation = {},
                EvolutionDex ownedEvolutionDex = {}, PartyMenuSelection allowPartyMenuSelection = {},
                WishCast successfulWish = {}, WishName sourceWishName = {}, ForcedSwitch ownForcedSwitch = {})
        : core(target), predicate(std::move(ownerHasNoMons)), otherTrainer(std::move(ownerIsOtherTrainer)),
          badge(std::move(ownerHasBadge)), captureFull(std::move(ownerCaptureFull)),
          ballThrow(std::move(executedBallThrow)), teamHealth(std::move(beforeTeamHealthCheck)),
          capturedMon(std::move(beforeCapturedMonGift)), evolution(std::move(ownerEvolution)), gameStatistic(std::move(ownedGameStatistic)),
          moneyAnnouncement(std::move(announcedMoney)), partyMenuOpen(std::move(beforePartyMenu)),
          partyMenuReturn(std::move(beforePartyMenuReshow)), partyMenuReply(std::move(normalizePartyMenuReply)),
          partyMenuSelection(std::move(allowPartyMenuSelection)),
          ballTextName(std::move(ownerBallTextName)), singleWild(std::move(isSingleWild)), singleTrainer(std::move(isSingleTrainer)),
          experienceCommand(std::move(nativeExperience)), evolutionGate(std::move(beforeEvolution)),
          evolutionQueue(std::move(nativeEvolutionQueue)), opponentReset(std::move(resetParticipation)),
          evolutionDex(std::move(ownedEvolutionDex)), wishCast(std::move(successfulWish)),
          wishName(std::move(sourceWishName)),forcedSwitch(std::move(ownForcedSwitch)), sites{{
              {WishCastAddress,0xBED8,{{0x6838,0x3006,0xE042,0x3F20}},bool(wishCast)},
              {WishTextAddress,0xBED7,{{0x1C21,0xF000,0xFCD5,0xF000}},bool(wishName)},
              {ForcedCountAddress,0xBED6,{{0x2F01,0xD805,0x480F,0x6800}},bool(forcedSwitch)},
              {ForcedChoiceAddress,0xBED5,{{0x4813,0x7801,0x0048,0x1980}},bool(forcedSwitch)},
              {PartyMenuSelectionAddress,0xBED9,{{0x490E,0x7008,0x490E,0x2001}},bool(partyMenuSelection)},
              {EvolutionDexAddress,0xBEDA,{{0xB500,0x480A,0x6800,0x7EC0}},bool(evolutionDex)},
              {ExperienceCommandAddress,0xBEDF,{{0xB5F0,0x464F,0x4646,0xB4C0}},bool(experienceCommand)},
              {EvolutionGateAddress,0xBEDE,{{0x4806,0x7800,0x2800,0xD003}},bool(evolutionGate)},
              {EvolutionQueueAddress,0xBEDD,{{0xB530,0x4814,0x7801,0x1C03}},bool(evolutionQueue)},
              {EvolutionQueueEndAddress,0xBEDC,{{0x4904,0x4804,0x6008,0xBC30}},bool(evolutionQueue)},
              {OpponentResetAddress,0xBEDB,{{0xB5F0,0x4647,0xB480,0x0600}},bool(opponentReset)},
              {HasNoMonsAddress, 0xBEF1, {{0xB5F0,0x4657,0x464E,0x4645}}, true},
              {IsOtherTrainerAddress, 0xBEF2, {{0xB530,0x1C0D,0x490F,0x680B}}, bool(otherTrainer)},
              {FlagGetAddress, 0xBEF3, {{0xB510,0x0400,0x0C04,0x1C20}}, bool(badge)},
              {CaptureFullAddress, 0xBEF4, {{0xB510,0x2400,0x2064,0x4360}}, bool(captureFull)},
              {BallThrowAddress, 0xBEF5, {{0x4A0B,0x490C,0x8828,0x0080}}, bool(ballThrow)},
              {CheckTeamsLostAddress, 0xBEF6, {{0xB5F0,0x4647,0xB480,0x2600}}, bool(teamHealth)},
              {GiveMonAddress, 0xBEF7, {{0xB570,0x1C06,0x4C09,0x6822}}, bool(capturedMon)},
              {EvolutionAddress, 0xBEF8, {{0xB5F0,0x4657,0x464E,0x4645}}, bool(evolution)},
              {IncrementGameStatAddress, 0xBEF9, {{0xB510,0x0600,0x0E04,0x2C33}}, bool(gameStatistic)},
              {TrainerMoneyTextAddress, 0xBEFA, {{0x490F,0x20FD,0x7008,0x2001}}, bool(moneyAnnouncement)},
              {PayDayTextAddress, 0xBEFB, {{0x490F,0x20FD,0x7008,0x2001}}, bool(moneyAnnouncement)},
              {PartyMenuOpenAddress,0xBEEC,{{0xB570,0xB081,0x482A,0x21FF}},bool(partyMenuOpen)},
              {PartyMenuReturnAddress,0xBEED,NativePartyMenuReshowPreimage,bool(partyMenuReturn)},
              {PartyMenuReturn2Address,0xBEEE,{{0xB500,0x4802,0xF7CF,0xFDB0}},bool(partyMenuReturn)},
              {PartyMenuReplyAddress,0xBEEF,NativeChosenMonReturnPreimage,bool(partyMenuReply)},
              {WildEscapeDoubleCheckAddress,0xBEEB,{{0x2001,0x4003,0x2B00,0xD120}},true},
              {PlayerNameTextAddress,0xBEEA,{{0x6804,0xE0CF,0x0000,0x4F5C}},bool(ballTextName)},
              {WildIntroSpriteAddress,0xBEE9,{{0x2000,0xF7FA,0xFEDB,0x4649}},bool(singleWild)},
              {WildIntroTextAddress,0xBEE8,{{0x2001,0x4008,0x2800,0xD003}},bool(singleWild)},
              {WildBallTargetAddress,0xBEE7,{{0x7030,0x480C,0x6801,0x2080}},bool(singleWild)},
              {TrainerIntroBattlerAddress,0xBEE6,{{0x4C20,0x7820,0xF061,0xFFFD}},bool(singleTrainer)},
              {TrainerIntroTextAddress,0xBEE5,{{0x2001,0x4008,0x2800,0xD015}},bool(singleTrainer)},
              {BattlerSpritePresentAddress,0xBEE4,{{0xB530,0x0600,0x0E04,0x1C25}},bool(singleWild)||bool(singleTrainer)},
              {IsDoubleBattleAddress,0xBEE3,{{0x4802,0x6800,0x2101,0x4008}},bool(singleWild)||bool(singleTrainer)},
              {ReshowLoadSpriteAddress,0xBEE2,{{0xB510,0x0600,0x0E04,0x4810}},bool(singleWild)||bool(singleTrainer)},
              {ReshowCreateSpriteAddress,0xBEE1,{{0xB5F0,0x4657,0x464E,0x4645}},bool(singleWild)||bool(singleTrainer)},
              {ReshowCreateHealthboxAddress,0xBEE0,{{0xB570,0x0600,0x0E05,0x4808}},bool(singleWild)||bool(singleTrainer)}}} {
        if (!core || !core->cpu || !core->rawRead16 || !core->rawWrite16 || !core->busRead8 ||
            !core->busRead16 || !core->busRead32 || ((ballTextName || singleWild || singleTrainer || wishName) && !core->busWrite8) || !predicate)
            throw std::runtime_error("Cannot install FireRed battle ownership hooks");
        cpu = static_cast<ARMCore*>(core->cpu);
        for (const auto& site : sites) if (site.enabled)
            for (unsigned i = 0; i < site.preimage.size(); ++i)
                if (core->rawRead16(core, site.address + i * 2, -1) != site.preimage[i])
                    throw std::runtime_error("FireRed battle hook has an unexpected native preimage");
        std::lock_guard<std::mutex> guard(registryMutex);
        if (registry.count(cpu) || cpu->irqh.bkpt16 == breakpoint)
            throw std::runtime_error("FireRed core already has battle ownership hooks");
        previous = cpu->irqh.bkpt16;
        registry.emplace(cpu, this);
        for (const auto& site : sites) if (site.enabled) {
            replaceInstruction(site, site.preimage[0], site.breakpoint);
            if (core->rawRead16(core, site.address, -1) != site.breakpoint) {
                for (const auto& cleanup : sites) if (cleanup.enabled)
                    replaceInstruction(cleanup, cleanup.breakpoint, cleanup.preimage[0]);
                registry.erase(cpu);
                throw std::runtime_error("Cannot patch FireRed battle instructions in memory");
            }
        }
        cpu->irqh.bkpt16 = breakpoint;
        attached = true;
    }
    ~BattleHooks() { detach(); }
    BattleHooks(const BattleHooks&) = delete;
    BattleHooks& operator=(const BattleHooks&) = delete;
    BattleHooks(BattleHooks&&) = delete;
    BattleHooks& operator=(BattleHooks&&) = delete;
    void check() const { if (error) std::rethrow_exception(error); }
    bool installed() const { return attached; }
    void detach() noexcept {
        if (!attached) return;
        std::lock_guard<std::mutex> guard(registryMutex);
        for (const auto& site : sites) if (site.enabled) replaceInstruction(site, site.breakpoint, site.preimage[0]);
        if (cpu->irqh.bkpt16 == breakpoint) cpu->irqh.bkpt16 = previous;
        registry.erase(cpu);
        attached = false;
    }
};
} // namespace coop::firered
