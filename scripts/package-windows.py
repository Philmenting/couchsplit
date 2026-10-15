#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Stage native MinGW applications, Qt/plugins, transitive DLLs and GPL source."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import urllib.request
import zipfile

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('maker',ROOT/'scripts/make-installer.py')
maker=importlib.util.module_from_spec(spec);spec.loader.exec_module(maker)

def stage(upstream,output,prefix):
    files,metadata=maker.source_files(ROOT,upstream,windows=True)
    output.mkdir(parents=True,exist_ok=False)
    for name,(data,mode) in files.items():
        target=output/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    binary=output/'project/build/bin'
    for name in ('platinum-coop.exe','melonDS-coop.exe'):
        subprocess.run([str(prefix/'bin/windeployqt6.exe'),'--release','--no-translations','--dir',str(binary),str(binary/name)],check=True)
    available={p.name.lower():p for p in (prefix/'bin').glob('*.dll')}
    system=Path(os.environ['SYSTEMROOT'])/'System32'
    pending=list(binary.rglob('*.exe'))+list(binary.rglob('*.dll'));seen=set();runtime=[]
    while pending:
        path=pending.pop()
        if path in seen:continue
        seen.add(path)
        result=subprocess.check_output(['objdump','-p',str(path)],text=True,errors='replace')
        for name in re.findall(r'DLL Name:\s*(\S+)',result):
            source=available.get(name.lower())
            if source:
                target=binary/source.name
                if not target.exists():
                    shutil.copyfile(source,target);pending.append(target);runtime.append(source.name)
            elif not (system/name).is_file() and not name.lower().startswith(('api-ms-','ext-ms-')):
                raise ValueError('Unresolved Windows dependency: '+name)
    # CPython is used only for the existing validated ROM/ZIP importer.
    archive=output/'python-embed.zip'
    urllib.request.urlretrieve('https://www.python.org/ftp/python/3.12.10/python-3.12.10-embed-amd64.zip',archive)
    with zipfile.ZipFile(archive) as source:source.extractall(binary/'python')
    archive.unlink()
    licenses=output/'dependency-licenses';licenses.mkdir()
    license_root=prefix/'share/licenses'
    if license_root.is_dir():shutil.copytree(license_root,licenses/'msys2',dirs_exist_ok=True)
    manifest=subprocess.check_output(['pacman','-Q'],text=True)
    (licenses/'MSYS2-PACKAGES.txt').write_text(manifest)
    (licenses/'SOURCES.md').write_text(
        '# Third-party runtime sources and licenses\n\n'
        'Qt and runtime DLLs are dynamically linked. Their licenses are included here; '
        'users may replace compatible DLLs. No DRM or relinking restriction is applied.\n\n'
        'Exact package versions are in MSYS2-PACKAGES.txt. Full package build recipes, patches, '
        'upstream source URLs and checksums: https://github.com/msys2/MINGW-packages . '
        'Historical binaries and source packages: https://repo.msys2.org/mingw/sources/ .\n'
        'Qt original source archives: https://download.qt.io/archive/qt/ .\n'
        'CPython 3.12.10 full source and license: https://www.python.org/ftp/python/3.12.10/ .\n')
    metadata['runtime_dlls']=sorted(runtime);metadata['packages']=manifest
    (output/'release.json').write_text(json.dumps(metadata,indent=2))
    (output/'Start.bat').write_text('@echo off\r\nstart "" "%~dp0project\\build\\bin\\platinum-coop.exe" --fullscreen --first-run\r\n')
    print(output)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--upstream',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--prefix',type=Path,required=True)
    a=p.parse_args();stage(a.upstream,a.output,a.prefix)
