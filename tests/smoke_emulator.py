#!/usr/bin/env python3
"""ROM-free production emulator smoke test using self-authored ARM homebrew.
No Nintendo logo, game content, BIOS or firmware bytes are included. This tests
actual DS execution, frame IPC, local TCP roles and clean process termination;
it is not a Pokémon/world synchronization test.
"""
from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile
import time
import socket
import mmap


def homebrew():
    # Tiny ARM programs: configure 2D backdrop colours, then loop indefinitely.
    instructions=[];literals=[];fixups=[]
    def load(register,value):
        fixups.append((len(instructions),register,len(literals)))
        instructions.append(0);literals.append(value)
    for address,value in [(0x04000304,0x8203),(0x04000000,0x10000),(0x04001000,0x10000),(0x05000000,0x001F),(0x05000400,0x03E0)]:
        load(0,address);load(1,value);instructions.append(0xE5801000)  # STR r1,[r0]
    instructions.append(0xEAFFFFFE)  # B .
    for index,register,slot in fixups:
        offset=4*(len(instructions)+slot-index)-8
        instructions[index]=0xE59F0000|(register<<12)|offset  # LDR literal
    arm9=struct.pack('<'+'I'*(len(instructions)+len(literals)),*(instructions+literals))
    arm7=struct.pack('<I',0xEAFFFFFE)
    rom=bytearray(128*1024)
    rom[:12]=b'COOPTEST    ';rom[12:16]=b'HBRW';rom[16:18]=b'00'
    struct.pack_into('<IIII',rom,0x20,0x200,0x02000000,0x02000000,len(arm9))
    struct.pack_into('<IIII',rom,0x30,0x1000,0x03800000,0x03800000,len(arm7))
    struct.pack_into('<II',rom,0x80,len(rom),0x200)
    rom[0x200:0x200+len(arm9)]=arm9;rom[0x1000:0x1000+len(arm7)]=arm7
    # Header CRC16 used by the standard DS header, over our own header only.
    crc=0xFFFF
    for byte in rom[:0x15E]:
        crc ^= byte
        for _ in range(8):crc=(crc>>1)^ (0xA001 if crc&1 else 0)
    struct.pack_into('<H',rom,0x15E,crc)
    return rom


def smoke(binary):
    binary=Path(binary).resolve()
    capabilities=subprocess.check_output([str(binary),'--coop-capabilities'],timeout=5)
    assert b'STABLE_SAVE=1' in capabilities, 'Update the launcher and emulator together'
    with tempfile.TemporaryDirectory(prefix='coop-emulator-ä-') as temp:
        base=Path(temp);rom=base/'self-authored-homebrew.nds';rom.write_bytes(homebrew())
        probe=socket.socket();probe.bind(('127.0.0.1',0));port=probe.getsockname()[1];probe.close()
        processes=[];logs=[];maps=[]
        try:
            for p,role in enumerate(('host','join')):
                profile=base/role;profile.mkdir()
                (profile/'saves').mkdir()
                (profile/'melonDS.toml').write_text('[Emu]\nDirectBoot = true\nExternalBIOSEnable = false\n[Screen]\nUseGL = false\n["3D"]\nRenderer = 0\n[Instance0]\nSaveFilePath = "'+(profile/'saves').as_posix()+'"\n',encoding='utf-8')
                ipc=base/(role+'.ipc')
                ipc.write_bytes(struct.pack('<10I',0x434F5031,2,0,0xFFF,0,0,0,0,0,0)+bytes(256*192*4*2));ipc.chmod(0o600);maps.append(ipc)
                env={k:v for k,v in os.environ.items() if not k.startswith(('MELONDS_AP','PMCOOP_'))}
                env.update(QT_QPA_PLATFORM='offscreen',SDL_AUDIODRIVER='dummy',PMCOOP_PROFILE=str(profile),PMCOOP_IPC=str(ipc),PMCOOP_ROLE=role,PMCOOP_PORT=str(port),PMCOOP_SAVE_NAME='game')
                log=open(base/(role+'.log'),'wb');logs.append(log)
                processes.append(subprocess.Popen([str(binary),str(rom)],env=env,cwd=profile,stdout=log,stderr=subprocess.STDOUT))
            end=time.monotonic()+20
            headers=[]
            while time.monotonic()<end:
                if any(p.poll() is not None for p in processes):raise AssertionError('Production emulator exited early')
                headers=[struct.unpack('<10I',path.read_bytes()[:40]) for path in maps]
                if all(h[5]>10 and h[6] for h in headers) and headers[1][7]==2:break
                time.sleep(0.1)
            else:raise AssertionError('No frames / loopback roles within 20 s: '+repr(headers))
            assert all(len(set(path.read_bytes()[40:]))>1 for path in maps), 'Framebuffer is still empty'
            for path in maps:
                with path.open('r+b') as file, mmap.mmap(file.fileno(),0,access=mmap.ACCESS_WRITE) as memory:
                    memory[36:40]=struct.pack('<I',1)
            for p in processes:assert p.wait(timeout=10)==0,'Emulator did not exit cleanly'
            print('PASS: two PRODUCTION DS processes executed self-authored ARM homebrew, published frames, connected on localhost, assigned roles 1/2, and exited via cooperative IPC shutdown')
        except Exception:
            for log in logs:log.flush()
            for role in ('host','join'):
                path=base/(role+'.log')
                if path.exists():print(role,path.read_text(errors='replace')[-12000:])
            raise
        finally:
            for p in processes:
                if p.poll() is None:
                    p.kill();p.wait(timeout=5)
            for log in logs:log.close()


if __name__=='__main__':smoke(sys.argv[1])
