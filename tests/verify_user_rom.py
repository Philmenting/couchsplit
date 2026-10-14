#!/usr/bin/env python3
"""Opt-in real-ROM boot check. Keep --output OUTSIDE the source repository.

No ROM is copied. BIOS/firmware use the emulator's free built-in replacements.
TCP connectivity is explicitly not a shared-world synchronization test.
"""
import argparse
import hashlib
import json
import mmap
import os
from pathlib import Path
import socket
import struct
import subprocess
import time
import zlib


def png(path, pixels):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    rows = bytearray()
    for y in range(384):
        rows.append(0)
        for x in range(256):
            b, g, r, _ = pixels[(y * 256 + x) * 4:(y * 256 + x + 1) * 4]
            rows.extend((r, g, b))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 256, 384, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))


def verify(binary, rom, output, seconds):
    binary, rom, output = Path(binary).resolve(), Path(rom).resolve(), Path(output).resolve()
    repository = Path(__file__).resolve().parents[1]
    if output == repository or repository in output.parents:
        raise ValueError('Private test output must be outside the repository')
    output.mkdir(mode=0o700, parents=True, exist_ok=False)
    with open(rom, 'rb') as source:
        digest = hashlib.file_digest(source, 'sha256').hexdigest()
    probe = socket.socket(); probe.bind(('127.0.0.1', 0))
    port = probe.getsockname()[1]; probe.close()
    processes, logs, handles, maps = [], [], [], []
    try:
        for player, role in enumerate(('host', 'join')):
            profile = output / role; profile.mkdir(mode=0o700)
            (profile / 'saves').mkdir()
            (profile / 'melonDS.toml').write_text('LimitFPS = true\n[Emu]\nDirectBoot = true\nExternalBIOSEnable = false\n[Screen]\nUseGL = false\n["3D"]\nRenderer = 0\n[Instance0]\nJoystickID = -1\nSaveFilePath = ' + json.dumps(str(profile / 'saves')) + '\n')
            ipc = output / (role + '.ipc')
            ipc.write_bytes(struct.pack('<9I', 0x434F5031, 1, 0, 0xFFF, 0, 0, 0, 0, 0) + bytes(256 * 192 * 4 * 2))
            ipc.chmod(0o600)
            handle = open(ipc, 'r+b'); handles.append(handle)
            maps.append(mmap.mmap(handle.fileno(), 0))
            env = {k: v for k, v in os.environ.items() if not k.startswith(('MELONDS_AP', 'PMCOOP_'))}
            env.update(QT_QPA_PLATFORM='offscreen', SDL_AUDIODRIVER='dummy', PMCOOP_PROFILE=str(profile), PMCOOP_IPC=str(ipc), PMCOOP_ROLE=role, PMCOOP_PORT=str(port))
            log = open(output / (role + '.log'), 'wb'); logs.append(log)
            processes.append(subprocess.Popen([str(binary), str(rom)], env=env, cwd=profile, stdout=log, stderr=subprocess.STDOUT))
        start = time.monotonic(); first_frames = [None, None]; last_frames = [None, None]
        bridge_ever = [False, False]; peers_ever = [False, False]
        while time.monotonic() - start < seconds:
            elapsed = time.monotonic() - start
            if any(p.poll() is not None for p in processes):
                raise RuntimeError('Emulator exited before boot check completed; inspect private logs')
            # Only player 1 receives A pulses; player 2 stays neutral.
            pressed = 10 <= elapsed < 20 and elapsed % 2 < 0.3
            struct.pack_into('<I', maps[0], 12, 0xFFE if pressed else 0xFFF)
            for i, mapping in enumerate(maps):
                header = struct.unpack_from('<9I', mapping)
                if elapsed > 5 and first_frames[i] is None: first_frames[i] = header[5]
                last_frames[i] = header[5]
                peers_ever[i] |= bool(header[6]); bridge_ever[i] |= bool(header[8])
            time.sleep(0.05)
        for mapping in maps: struct.pack_into('<I', mapping, 12, 0xFFF)
        for process in processes: process.terminate()
        exits = [process.wait(timeout=10) for process in processes]
        headers = [list(struct.unpack_from('<9I', mapping)) for mapping in maps]
        frames = [bytes(mapping[36:]) for mapping in maps]
        for i, frame in enumerate(frames): png(output / ('player' + str(i + 1) + '.png'), frame)
        result = dict(rom_sha256=digest, seconds=seconds, frame_counts=last_frames,
                      frame_progress_after_5s=[last_frames[i] - first_frames[i] for i in range(2)],
                      tcp_connected=peers_ever, bridge_activated=bridge_ever,
                      final_roles=[h[7] for h in headers], exit_codes=exits,
                      final_views_differ=frames[0] != frames[1],
                      input='A pulses to player 1 only, 10–20 seconds',
                      world_synchronization_verified=False)
        (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result, indent=2))
        if exits != [0, 0] or not all(peers_ever) or not all(n > 10 for n in result['frame_progress_after_5s']):
            raise RuntimeError('Boot/transport/lifecycle check failed; inspect private output')
    finally:
        for process in processes:
            if process.poll() is None: process.kill(); process.wait(timeout=5)
        for mapping in maps: mapping.close()
        for handle in handles + logs: handle.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary'); parser.add_argument('rom'); parser.add_argument('--output', required=True)
    parser.add_argument('--seconds', type=int, default=30)
    args = parser.parse_args()
    if args.seconds < 25: parser.error('--seconds must be at least 25')
    verify(args.binary, args.rom, args.output, args.seconds)
