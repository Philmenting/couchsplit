#!/usr/bin/env python3
"""Compile the production emulator asset-path function; no ROM or Qt stubs."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def test(source):
    text = (source / 'src/frontend/qt_sdl/EmuInstance.cpp').read_text(encoding='utf-8')
    start = text.index('string EmuInstance::getAssetPath(')
    end = text.index('\n}\n', start) + 3
    function = text[start:end]
    prelude = r'''
#include <string>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>
using std::string;
struct EmuInstance {
    string baseROMDir="/roms", baseGBAROMDir="/gba";
    string baseAssetName="original", baseGBAAssetName="gba-game";
    string getAssetPath(bool, const string&, const string&, const string&);
};
'''
    checks = r'''
void saveMode(const char* value) {
#ifdef _WIN32
    _putenv_s("PMCOOP_SAVE_NAME",value);
#else
    if(*value) setenv("PMCOOP_SAVE_NAME",value,1);
    else unsetenv("PMCOOP_SAVE_NAME");
#endif
}
int main() {
    EmuInstance instance;
    saveMode("");
    assert(instance.getAssetPath(false,"/saves/",".sav")=="/saves/original.sav");
    saveMode("game");
    assert(instance.getAssetPath(false,"/saves/",".sav")=="/saves/game.sav");
    instance.baseAssetName="renamed";
    assert(instance.getAssetPath(false,"/saves/",".sav")=="/saves/game.sav");
    assert(instance.getAssetPath(false,"C:\\Saves\\",".sav")=="C:\\Saves/game.sav");
    assert(instance.getAssetPath(false,"/states",".mln")=="/states/renamed.mln");
    assert(instance.getAssetPath(false,"/cheats",".mch")=="/cheats/renamed.mch");
    assert(instance.getAssetPath(true,"/gba-saves",".sav")=="/gba-saves/gba-game.sav");
    assert(instance.getAssetPath(false,"/saves",".sav","explicit")=="/saves/explicit.sav");
    saveMode("../unexpected");
    assert(instance.getAssetPath(false,"/saves",".sav")=="/saves/renamed.sav");
    saveMode("");
    assert(instance.getAssetPath(false,"/saves",".sav")=="/saves/renamed.sav");
    puts("PASS: production save path survives ROM renaming; ordinary emulator, GBA, savestate and cheat paths preserved");
}
'''
    with tempfile.TemporaryDirectory(prefix='coop-save-paths-') as temporary:
        root = Path(temporary)
        cpp = root / 'paths.cpp'
        binary = root / ('paths.exe' if os.name == 'nt' else 'paths')
        cpp.write_text(prelude + function + checks, encoding='utf-8')
        subprocess.run(['g++', '-std=c++17', '-O1', '-UNDEBUG', '-Wall', '-Wextra', str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    test(parser.parse_args().source)
