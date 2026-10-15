# SPDX-License-Identifier: GPL-3.0-or-later
"""Small binary readers/writers for user-supplied DS/NARC/message data.

No game assets or original game text are bundled. Message-bank format reference:
https://github.com/pret/pokeplatinum/blob/main/src/message.c
"""
import struct


def span(data, offset, size):
    if offset < 0 or size < 0 or offset + size > len(data):
        raise ValueError('Binary section lies outside its file')
    return data[offset:offset + size]


def crc16(data):
    value = 0xFFFF
    for byte in data:
        value ^= byte
        for _ in range(8): value = (value >> 1) ^ (0xA001 if value & 1 else 0)
    return value


class Rom:
    def __init__(self, data):
        self.data = bytes(data)
        span(self.data, 0, 512)
        fnt_offset, fnt_size, self.fat_offset, fat_size = struct.unpack_from('<4I', data, 0x40)
        fnt = span(data, fnt_offset, fnt_size)
        fat = span(data, self.fat_offset, fat_size)
        if fat_size % 8: raise ValueError('Invalid ROM FAT size')
        self.fat = [struct.unpack_from('<II', fat, i) for i in range(0, fat_size, 8)]
        for start, end in self.fat: span(data, start, end - start)
        self.names = {}
        visited = set()
        def directory(number, prefix):
            if number in visited: raise ValueError('Cyclic ROM directory table')
            visited.add(number)
            table = span(fnt, (number - 0xF000) * 8, 8)
            position, file_id, _ = struct.unpack('<IHH', table)
            while True:
                length = span(fnt, position, 1)[0]; position += 1
                if not length: break
                name = span(fnt, position, length & 127).decode('ascii'); position += length & 127
                if name in ('.', '..') or '/' in name: raise ValueError('Invalid ROM filename')
                path = prefix + name
                if length & 128:
                    child = struct.unpack('<H', span(fnt, position, 2))[0]; position += 2
                    directory(child, path + '/')
                else:
                    if path in self.names or file_id >= len(self.fat): raise ValueError('Invalid ROM filename index')
                    self.names[path] = file_id; file_id += 1
        directory(0xF000, '')

    def file(self, name):
        start, end = self.fat[self.names[name]]
        return self.data[start:end]

    def replace(self, files):
        # Keep ARM binaries, overlays and all old payload bytes intact. Only append
        # new archives and repoint their FAT entries; never rebuild the game code.
        output = bytearray(self.data)
        for name, contents in files.items():
            output.extend(b'\xff' * (-len(output) % 4))
            start = len(output); output.extend(contents)
            struct.pack_into('<II', output, self.fat_offset + self.names[name] * 8, start, len(output))
        if len(output) > 512 * 1024 * 1024: raise ValueError('Result exceeds DS ROM size limit')
        capacity = 0
        while (128 * 1024 << capacity) < len(output): capacity += 1
        output[0x14] = capacity
        struct.pack_into('<I', output, 0x80, len(output))
        struct.pack_into('<H', output, 0x15E, crc16(output[:0x15E]))
        # Check every referenced resource after writing; all other files must stay exact.
        reread = Rom(output)
        replaced_ids = {self.names[name]: contents for name, contents in files.items()}
        for i, (start, end) in enumerate(self.fat):
            a, b = reread.fat[i]
            if output[a:b] != replaced_ids.get(i, self.data[start:end]):
                raise ValueError('Unexpected resource change: FAT member ' + str(i))
        for header_offset in (0x20, 0x30):
            offset, _, _, size = struct.unpack_from('<4I', self.data, header_offset)
            if output[offset:offset + size] != self.data[offset:offset + size]:
                raise ValueError('ARM executable changed')
        return bytes(output)


class Narc:
    def __init__(self, data):
        if span(data, 0, 4) != b'NARC': raise ValueError('Not a NARC archive')
        _, _, total, header_size, count = struct.unpack_from('<HHIHH', data, 4)
        if total != len(data) or header_size != 16 or count != 3: raise ValueError('Unsupported NARC header')
        blocks = {}; position = header_size
        for _ in range(count):
            tag = bytes(span(data, position, 4))
            size = struct.unpack('<I', span(data, position + 4, 4))[0]
            if size < 8 or tag in blocks: raise ValueError('Invalid NARC block')
            blocks[tag] = bytes(span(data, position, size)); position += size
        if position != len(data) or set(blocks) != {b'BTAF', b'BTNF', b'GMIF'}: raise ValueError('Invalid NARC blocks')
        self.header = bytes(data[:16]); self.names = blocks[b'BTNF']
        allocation = blocks[b'BTAF']; image = blocks[b'GMIF'][8:]
        count = struct.unpack_from('<H', allocation, 8)[0]
        if len(allocation) != 12 + count * 8: raise ValueError('Invalid NARC allocation table')
        self.files = []
        for i in range(count):
            start, end = struct.unpack_from('<II', allocation, 12 + i * 8)
            self.files.append(bytes(span(image, start, end - start)))

    def save(self, files=None):
        files = self.files if files is None else files
        if len(files) > 65535: raise ValueError('Too many NARC members')
        image = bytearray(); entries = bytearray()
        for data in files:
            image.extend(b'\xff' * (-len(image) % 4))
            start = len(image); image.extend(data)
            entries.extend(struct.pack('<II', start, len(image)))
        image.extend(b'\xff' * (-len(image) % 4))
        allocation = b'BTAF' + struct.pack('<IHH', 12 + len(entries), len(files), 0) + entries
        image_block = b'GMIF' + struct.pack('<I', 8 + len(image)) + image
        header = bytearray(self.header)
        struct.pack_into('<I', header, 8, 16 + len(allocation) + len(self.names) + len(image_block))
        return bytes(header) + allocation + self.names + image_block


class Bank:
    def __init__(self, data):
        self.original = bytes(data)
        count, self.seed = struct.unpack('<HH', span(data, 0, 4))
        span(data, 4, count * 8)
        self.entries = []
        for i in range(count):
            key = (self.seed * 765 * (i + 1)) & 0xFFFF; key |= key << 16
            offset, length = struct.unpack_from('<II', data, 4 + 8 * i)
            raw = span(data, offset ^ key, (length ^ key) * 2)
            encrypted = struct.unpack('<' + 'H' * (len(raw) // 2), raw)
            self.entries.append(tuple(c ^ (((i + 1) * 596947 + j * 18749) & 0xFFFF) for j, c in enumerate(encrypted)))

    def save(self, entries=None):
        entries = self.entries if entries is None else entries
        if len(entries) > 65535: raise ValueError('Too many message entries')
        header = bytearray(struct.pack('<HH', len(entries), self.seed)); table = bytearray(); payload = bytearray()
        for i, text in enumerate(entries):
            offset = 4 + len(entries) * 8 + len(payload)
            key = (self.seed * 765 * (i + 1)) & 0xFFFF; key |= key << 16
            table.extend(struct.pack('<II', offset ^ key, len(text) ^ key))
            codes = [c ^ (((i + 1) * 596947 + j * 18749) & 0xFFFF) for j, c in enumerate(text)]
            payload.extend(struct.pack('<' + 'H' * len(codes), *codes))
        return bytes(header + table + payload)
