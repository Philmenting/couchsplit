#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Native Zorin OS 18 / Ubuntu 24.04 amd64 package, including matching GPL source."""
import argparse
import importlib.util
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('maker',ROOT/'scripts/make-installer.py')
maker=importlib.util.module_from_spec(spec);spec.loader.exec_module(maker)
spec=importlib.util.spec_from_file_location('runtime',ROOT/'scripts/install-bazzite.py')
runtime=importlib.util.module_from_spec(spec);spec.loader.exec_module(runtime)

def package(upstream,output,version):
    if not version or any(c not in '0123456789abcdefghijklmnopqrstuvwxyz.+~-:' for c in version):
        raise ValueError('Invalid Debian version')
    files,metadata=maker.source_files(ROOT,upstream)
    output=output.resolve()
    with tempfile.TemporaryDirectory(prefix='coop-deb-') as temporary:
        stage=Path(temporary)
        for name,(data,mode) in files.items():
            path=stage/'opt/pokemon-platinum-local-coop'/name
            path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data);path.chmod(mode)
        control=stage/'DEBIAN/control';control.parent.mkdir()
        dependencies=['libc6 (>= 2.39)','libstdc++6','python3',*runtime.PACKAGES]
        control.write_text(
            'Package: pokemon-platinum-local-coop\nVersion: '+version+
            '\nArchitecture: amd64\nMaintainer: Platinum Local Co-op contributors\n'
            'Section: games\nPriority: optional\nDepends: '+', '.join(dict.fromkeys(dependencies))+
            '\nHomepage: https://github.com/Philmenting/pokemon-platinum-local-coop\n'
            'Description: Two-player Project PM launcher for Zorin OS 18\n'
            ' Own compatible patched ROM required. No Nintendo files included.\n'
            ' Complete matching GPL source is installed alongside the application.\n')
        desktop=stage/'usr/share/applications/pokemon-platinum-local-coop.desktop'
        desktop.parent.mkdir(parents=True)
        desktop.write_text('[Desktop Entry]\nType=Application\nName=Pokémon Platinum Local Co-op\n'
            'Comment=Zwei Spieler mit Project PM\n'
            'Exec=/opt/pokemon-platinum-local-coop/project/build/bin/platinum-coop --fullscreen --first-run\n'
            'Terminal=false\nIcon=applications-games\nCategories=Game;Emulator;\n',encoding='utf-8')
        copyright=stage/'usr/share/doc/pokemon-platinum-local-coop/copyright'
        copyright.parent.mkdir(parents=True)
        copyright.write_text('GPL-3.0-or-later. See /opt/pokemon-platinum-local-coop/project/LICENSE.\n'
            'Complete matching upstream source: /opt/pokemon-platinum-local-coop/corresponding-source\n'
            'Project commit: '+metadata['project_commit']+'\n')
        subprocess.run(['dpkg-deb','--root-owner-group','--build',str(stage),str(output)],check=True)
    subprocess.run(['dpkg-deb','--info',str(output)],check=True)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--upstream',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--version',default='0.2.0~alpha.5')
    a=p.parse_args();package(a.upstream,a.output,a.version)
