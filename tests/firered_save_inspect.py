# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only, exact FireRed battery inspection for opt-in private acceptance.

No ROM data, save data, memory-writing API, or emulator dependency is included.
Reject partial/duplicate sectors and invalid party checksums before comparing
owned progress. The native format has two rotating fourteen-sector generations.
"""
from itertools import permutations
import hashlib
import struct


def demand(condition, message):
    if not condition:
        raise ValueError(message)


def u16(data, offset):
    return struct.unpack_from('<H', data, offset)[0]


def u32(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def latest_blocks(data):
    demand(len(data) == 0x20000, 'Battery must contain exactly 128 KiB')
    generations = []
    for slot in range(2):
        sections, counters = {}, set()
        for sector in range(14):
            block = data[(slot * 14 + sector) * 0x1000: (slot * 14 + sector + 1) * 0x1000]
            identifier = u16(block, 0xFF4)
            if identifier >= 14 or identifier in sections or u32(block, 0xFF8) != 0x08012025:
                break
            total = 0xF24 if identifier == 0 else 0x3D68 if identifier < 5 else 0x83D0
            chunk = 0 if identifier == 0 else identifier - 1 if identifier < 5 else identifier - 5
            length = min(0xF80, total - chunk * 0xF80)
            checksum = sum(struct.unpack_from('<' + 'I' * (length // 4), block)) & 0xFFFFFFFF
            if u16(block, 0xFF6) != ((checksum >> 16) + checksum) & 0xFFFF:
                break
            sections[identifier] = block[:length]
            counters.add(u32(block, 0xFFC))
        if len(sections) == 14 and len(counters) == 1:
            generations.append((counters.pop(), sections))
    demand(generations, 'No complete checksum-valid native save generation')
    generation, sections = generations[0]
    if len(generations) == 2 and 0 < (generations[1][0] - generation) & 0xFFFFFFFF < 0x80000000:
        generation, sections = generations[1]
    return generation, sections[0], b''.join(sections[i] for i in range(1, 5)), b''.join(sections[i] for i in range(5, 14))


def pokemon(record):
    demand(len(record) == 100, 'Invalid party record length')
    identity = u32(record, 0), u32(record, 4)
    order = list(permutations(range(4)))[identity[0] % 24]
    plain = bytearray(48)
    for physical, logical in enumerate(order):
        for word in range(3):
            value = u32(record, 32 + physical * 12 + word * 4) ^ identity[0] ^ identity[1]
            struct.pack_into('<I', plain, logical * 12 + word * 4, value)
    demand(sum(struct.unpack('<24H', plain)) & 0xFFFF == u16(record, 28), 'Pokemon secure checksum is invalid')
    species, held = u16(plain, 0), u16(plain, 2)
    level, hp, maximum = record[84], u16(record, 86), u16(record, 88)
    demand(0 < species <= 411 and 0 < level <= 100 and 0 < maximum <= 999 and hp <= maximum, 'Invalid party species, level or HP')
    return dict(identity=identity, species=species, held=held, experience=u32(plain, 4), level=level,
                hp=hp, maximum_hp=maximum, pp=tuple(plain[20:24]), bytes=record.hex())


def inspect_save(data):
    generation, block2, block1, storage = latest_blocks(data)
    key = u32(block2, 0xF20)
    count = block1[0x34]
    demand(0 < count <= 6, 'Native party count is outside 1..6')
    party = [pokemon(block1[0x38 + i * 100:0x38 + (i + 1) * 100]) for i in range(count)]
    demand(len({m['identity'] for m in party}) == count, 'Party contains duplicate identities')
    wallet = u32(block1, 0x290) ^ key
    demand(wallet <= 999999, 'Invalid native wallet')
    statistics = tuple(u32(block1, 0x121C + i * 4) ^ key for i in range(3))
    demand(all(n <= 0xFFFFFF for n in statistics), 'Invalid battle counters')
    directions = [block1[0x6A0 + i * 0x24 + 0x18] & 15 for i in range(16)
                  if block1[0x6A0 + i * 0x24] & 1 and block1[0x6A0 + i * 0x24 + 8] == 0xFF]
    return dict(generation=generation, map=tuple(block1[4:6]), position=struct.unpack_from('<hh', block1),
                party=party, wallet=wallet, battles=statistics, facing=directions[0] if len(directions) == 1 else None,
                storage_sha256=hashlib.sha256(storage).hexdigest(),
                trainer=block2[:9].hex() + block2[0xA:0xE].hex())


def check_owned_progress(initial, current):
    demand(current['trainer'] == initial['trainer'], 'Trainer identity changed')
    demand([m['identity'] for m in current['party']] == [m['identity'] for m in initial['party']], 'Native save contains missing, borrowed or reordered Pokemon')
    demand(current['storage_sha256'] == initial['storage_sha256'], 'PC storage changed during battle-only acceptance')
    demand(current['wallet'] == initial['wallet'], 'Wild battles changed the authored wallet')
    for before, after in zip(initial['party'], current['party']):
        demand(after['experience'] >= before['experience'], 'Owned Pokemon lost experience')
        demand(after['held'] == before['held'], 'Owned held item changed')
    for before, after in zip(initial['battles'], current['battles']):
        demand(after >= before, 'Native battle statistic decreased')
