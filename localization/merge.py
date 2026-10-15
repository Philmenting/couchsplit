# SPDX-License-Identifier: GPL-3.0-or-later
"""Three-way localization: retain PM structure, import German vanilla text,
and apply explicit translations for PM-changed strings. All entry IDs remain.
"""
from collections import Counter, defaultdict
import hashlib
import re
import struct
from .formats import Bank, Narc
from .text import controls, decode, encode, expand

ARCHIVES = ('msgdata/msg.narc', 'msgdata/pl_msg.narc')


def text_hash(words):
    words = expand(words)
    return hashlib.sha256(struct.pack('<' + 'H' * len(words), *words)).hexdigest()


def canonical(words):
    return re.sub(r'\s+', ' ', decode(words).strip()).replace('’', "'").replace('…', '...')


def localize(us, german, pm, translations):
    result = {}; stats = Counter(); unresolved = []
    overrides = translations.get('messages', {})
    keep = set(translations.get('keep', []))
    for path in ARCHIVES:
        archives = {k: Narc(r.file(path)) for k, r in [('us', us), ('de', german), ('pm', pm)]}
        if len({len(a.files) for a in archives.values()}) != 1: raise ValueError('Text bank indexes differ: ' + path)
        originals = [Bank(b) for b in archives['us'].files]
        Germans = [Bank(b) for b in archives['de'].files]
        global_lookup = defaultdict(set); local_lookup = []
        for ub, db in zip(originals, Germans):
            local = defaultdict(set)
            for u, d in zip(ub.entries, db.entries):
                local[canonical(u)].add(expand(d)); global_lookup[canonical(u)].add(expand(d))
            local_lookup.append(local)
        files = []
        for bank_id, raw in enumerate(archives['pm'].files):
            bank = Bank(raw); u = originals[bank_id]; d = Germans[bank_id]; entries = []
            # The unused Diamond/Pearl credits have 216 US versus 237 DE rows.
            # Region-specific credit insertions shift IDs. Preserve the original
            # credits rather than truncating the German list into US indexes.
            if path == 'msgdata/msg.narc' and bank_id == 493:
                files.append(raw)
                stats['entries'] += len(bank.entries)
                stats['legacy_credit_entries_retained'] += len(bank.entries)
                continue
            if len(u.entries) != len(d.entries):
                raise ValueError(f'Regional message indexes differ: {path}:{bank_id}')
            for entry_id, words in enumerate(bank.entries):
                stats['entries'] += 1; translated = words; key = text_hash(words)
                if key in overrides:
                    translated = encode(overrides[key], bool(words and words[0] == 0xF100))
                    if controls(translated) != controls(words): raise ValueError(f'Control sequence changed: {path}:{bank_id}:{entry_id}')
                    stats['explicit_pm_translation'] += 1
                elif entry_id < len(u.entries) and expand(words) == expand(u.entries[entry_id]) and entry_id < len(d.entries):
                    translated = d.entries[entry_id]; stats['german_vanilla_same_id'] += 1
                else:
                    candidates = local_lookup[bank_id].get(canonical(words), set()) or global_lookup.get(canonical(words), set())
                    if len(candidates) == 1:
                        # Reordered trainer dialogue/names reuse the corresponding German original.
                        value = next(iter(candidates))
                        translated = encode(decode(value), bool(words and words[0] == 0xF100))
                        stats['german_vanilla_reordered'] += 1
                    elif key in keep or not decode(words).strip(): stats['intentional_neutral'] += 1
                    else:
                        unresolved.append(dict(archive=path, bank=bank_id, entry=entry_id, source_hash=key, text=decode(words)))
                        stats['unresolved'] += 1
                # No message count changes, no script/party/control-block data changes.
                entries.append(tuple(translated))
            encoded = raw if entries == bank.entries else bank.save(entries)
            check = Bank(encoded)
            if check.entries != entries: raise ValueError('Message round-trip failed')
            files.append(encoded)
        saved = archives['pm'].save(files)
        if Narc(saved).files != files: raise ValueError('Archive round-trip failed')
        result[path] = saved
    # Font metrics from the German original are safe only where PM made no modifications.
    for path in ('graphic/font.narc', 'graphic/pl_font.narc'):
        if pm.file(path) == us.file(path): result[path] = german.file(path); stats['german_font_archives'] += 1
    return result, dict(stats), unresolved
