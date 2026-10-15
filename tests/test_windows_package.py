#!/usr/bin/env python3
"""Verify deployed native applications and embedded importer without MSYS2 in PATH."""
from pathlib import Path
import hashlib
import os
import subprocess
import sys
import tempfile
import zipfile

def main():
    root=Path(sys.argv[1]).resolve();binary=root/'project/build/bin'
    env=dict(os.environ,PATH=str(Path(os.environ['SYSTEMROOT'])/'System32'),
             QT_QPA_PLATFORM='offscreen',SDL_AUDIODRIVER='dummy',QT_LOGGING_TO_CONSOLE='1')
    for key in ('QT_PLUGIN_PATH','QML2_IMPORT_PATH','PYTHONPATH','PYTHONHOME'):env.pop(key,None)
    with tempfile.TemporaryDirectory(prefix='coop package ä ') as temporary:
        data=Path(temporary);source=data/'own synthetic.nds';header=bytearray(4096);header[12:16]=b'CPUE';source.write_bytes(header)
        archive=data/'own.zip'
        with zipfile.ZipFile(archive,'w') as zipped:zipped.write(source,'own.nds')
        result=subprocess.run([str(binary/'python/python.exe'),'-X','utf8',str(root/'project/scripts/import-rom.py'),
            '--rom',str(archive),'--into',str(data/'roms')],check=True,capture_output=True,text=True,encoding='utf-8',env=env,timeout=30)
        imported=Path(result.stdout.strip());assert imported.is_file()
        assert hashlib.sha256(imported.read_bytes()).digest()==hashlib.sha256(header).digest()
        image=data/'launcher.png'
        subprocess.run([str(binary/'platinum-coop.exe'),'--data-dir',str(data/'profiles'),
            '--smoke-test',str(image)],check=True,env=env,timeout=30)
        assert image.is_file()
    subprocess.run([str(binary/'python/python.exe'),'-X','utf8',str(root/'project/tests/smoke_emulator.py'),
        str(binary/'melonDS-coop.exe')],check=True,env=env,timeout=45)
    print('PASS: deployed Qt app and embedded Python ROM/ZIP import, isolated PATH and Unicode paths')

if __name__=='__main__': main()
