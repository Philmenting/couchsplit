# SPDX-License-Identifier: GPL-3.0-or-later
"""Western DS glyphs and lossless control tokens for localization authoring."""
import re

GLYPHS = {0x121 + i: c for i, c in enumerate('0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz')}
GLYPHS.update({0x15F + i: chr(0xC0 + i) for i in range(64)})
GLYPHS.update({0x19F + i: c for i, c in enumerate('ŒœŞşªº¹²³$¡¿!?,.…·/‘’“”„《》()♂♀+-*#=&~:;♠♣♥♦★◉●■▲◆@♪%☀☁☂☃☺♚♛☹↗↘☽ ⁴₧₦ ')})
REVERSE = {v: k for k, v in GLYPHS.items()}
REVERSE.update({"'": 0x1B3, '"': 0x1B5, '\n': 0xE000})
TOKEN = re.compile(r'\{([0-9A-Fa-f]{4}(?:,[0-9A-Fa-f]{4})*)\}')


def expand(words):
    if not words or words[0] != 0xF100: return tuple(words)
    out = []; buffer = 0; bits = 0
    for word in words[1:]:
        buffer |= (word & 0x7FFF) << bits; bits += 15
        while bits >= 9:
            glyph = buffer & 511; buffer >>= 9; bits -= 9
            if glyph == 511: return tuple(out + [0xFFFF])
            out.append(glyph)
    raise ValueError('Unterminated compressed name')


def compress(words):
    output = [0xF100]; buffer = 0; bits = 0
    for word in list(words[:-1]) + [511]:
        if word > 511: raise ValueError('Control code in compressed name')
        buffer |= word << bits; bits += 9
        while bits >= 15:
            output.append(buffer & 0x7FFF); buffer >>= 15; bits -= 15
    if bits: output.append((buffer | (0x7FFF << bits)) & 0x7FFF)
    return tuple(output + [0xFFFF])


def decode(words):
    words = expand(words); text = []; i = 0
    while i < len(words):
        code = words[i]; i += 1
        if code == 0xFFFF: break
        if code == 0xFFFE:
            if i + 2 > len(words): raise ValueError('Short control sequence')
            count = words[i + 1]
            if i + count + 2 > len(words): raise ValueError('Short control arguments')
            values = [code] + list(words[i:i + count + 2]); i += count + 2
            text.append('{' + ','.join(f'{c:04X}' for c in values) + '}')
        elif code == 0xE000: text.append('\n')
        else: text.append(GLYPHS.get(code, '{' + f'{code:04X}' + '}'))
    return ''.join(text)


def encode(text, packed=False):
    words = []; i = 0
    while i < len(text):
        match = TOKEN.match(text, i)
        if match:
            words.extend(int(v, 16) for v in match[1].split(',')); i = match.end()
        else:
            if text[i] not in REVERSE: raise ValueError('Unsupported DS glyph: ' + repr(text[i]))
            words.append(REVERSE[text[i]]); i += 1
    words.append(0xFFFF)
    return compress(words) if packed else tuple(words)


def controls(words):
    # Line breaks may move during translation. Runtime commands and parameters must not.
    return tuple(TOKEN.findall(decode(words)))
