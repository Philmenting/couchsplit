#!/usr/bin/env python3
"""Package owned git history, source, Linux binaries and matching GPL source.
Does not include ROMs, firmware, saves, build libraries, or upstream executables.
"""
import argparse
from pathlib import Path
import subprocess
import zipfile


def package(project, upstream, output):
    if subprocess.check_output(['git','-C',str(project),'status','--porcelain']).strip():
        raise SystemExit('Commit reviewed project changes before packaging.')
    binaries=[project/'build/bin/platinum-coop',project/'build/bin/melonDS-coop']
    if not all(p.is_file() for p in binaries):raise SystemExit('Build both release binaries first.')
    forbidden={'.nds','.srl','.sav','.dsv','.rom','.bios','.bin','.exe','.dll'}
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        tracked=subprocess.check_output(['git','-C',str(project),'ls-files','-z']).decode().split('\0')
        for name in filter(None,tracked):
            path=project/name
            if path.suffix.lower() in forbidden:raise SystemExit('Unexpected protected/binary file: '+name)
            z.write(path,'couchsplit/'+name)
        # This repository has no remote credentials. Preserve its reviewable commits.
        for path in (project/'.git').rglob('*'):
            if path.is_file() and path.name not in {'index.lock','COMMIT_EDITMSG'}:z.write(path,'couchsplit/'+str(path.relative_to(project)))
        for path in binaries:z.write(path,'linux-build/'+path.name)
        tracked_upstream=subprocess.check_output(['git','-C',str(upstream),'ls-files','-z']).decode().split('\0')
        for name in filter(None,tracked_upstream):
            path=upstream/name
            if path.suffix.lower() not in forbidden and path.is_file():z.write(path,'upstream-corresponding-source/'+name)
        for name in ('SocketCompat.h','CoopIPC.h'):
            z.write(upstream/'src/frontend/qt_sdl'/name,'upstream-corresponding-source/src/frontend/qt_sdl/'+name)
        z.writestr('upstream-corresponding-source/UPSTREAM_COMMIT','4d9785468bbefc8d08002f16df4d85a12f9611f7\nwith patches/project-pm-linux-coop.patch applied\n')
        z.writestr('START-HERE.txt','''CouchSplit — Linux prototype

1. Read couchsplit/README.md and docs/development-status.md.
2. On Bazzite run couchsplit/scripts/bazzite-build.sh.
3. Use your own legally provided compatible Project-PM patched Platinum ROM.

The linux-build binaries were compiled/tested on Ubuntu 24.04 x86_64 with
Qt6.4 / SDL2.30. Runtime libraries are NOT bundled. These binaries are NOT
claimed to be portable to Bazzite; build in Distrobox for the target system.
Their matching GPL corresponding source is provided in
upstream-corresponding-source and couchsplit.

The owned project directory includes its .git history.
Repository: https://github.com/Philmenting/couchsplit
Do not claim shared Sinnoh-world gameplay or Bazzite hardware validation:
those tests require the compatible ROM, physical controllers and target PC.

No Nintendo ROM, BIOS, proprietary firmware or user saves are included.
''')
    return output


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upstream',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();package(Path(__file__).resolve().parents[1],args.upstream.resolve(),args.output.resolve())
