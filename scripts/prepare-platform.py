#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply the small, idempotent ABI-2 additions after the pinned base patch."""
import argparse
from pathlib import Path

CHANGES = {
 'CMakeLists.txt': [
  ("pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)", "if(WIN32)\n    find_package(SDL2 CONFIG REQUIRED)\n    add_compile_definitions(SDL_MAIN_HANDLED)\n    set(COOP_SDL_TARGET SDL2::SDL2)\nelse()\n    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)\n    set(COOP_SDL_TARGET PkgConfig::SDL2)\nendif()"),
  ("fix_interface_includes(PkgConfig::SDL2 PkgConfig::LibArchive)", "if(WIN32)\n    fix_interface_includes(PkgConfig::LibArchive)\nelse()\n    fix_interface_includes(PkgConfig::SDL2 PkgConfig::LibArchive)\nendif()"),
  ("target_link_libraries(melonDS PRIVATE PkgConfig::SDL2 PkgConfig::LibArchive PkgConfig::Zstd PkgConfig::Faad)", "target_link_libraries(melonDS PRIVATE ${COOP_SDL_TARGET} PkgConfig::LibArchive PkgConfig::Zstd PkgConfig::Faad)"),
 ],
 'main.cpp': [
  ('SDL_SetHint(SDL_HINT_APP_NAME, "melonDS");',
   'SDL_SetMainReady();\n    SDL_SetHint(SDL_HINT_APP_NAME, "melonDS");'),
  ('#include <QTimer>', '#include <QTimer>\n#include "CoopIPC.h"'),
  ('if (const char* profile = getenv("PMCOOP_PROFILE")) {\n        emuDirectory = QString::fromUtf8(profile);',
   'if (const auto profile = qEnvironmentVariable("PMCOOP_PROFILE"); !profile.isEmpty()) {\n        emuDirectory = profile;'),
  ('PMCOOP_ABI=1;BRIDGE_WIRE=2;POSIX=1','PMCOOP_ABI=2;BRIDGE_WIRE=2'),
  ('PMCOOP_ABI=2;BRIDGE_WIRE=2','PMCOOP_ABI=2;BRIDGE_WIRE=2;STABLE_SAVE=1'),
  ('qputenv("QT_QPA_PLATFORM", "windows:darkmode=2");',
   'if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "windows:darkmode=2");'),
  ('if (getenv("PMCOOP_IPC")) {\n        emuInstances',
   'coop::Mapping couchControl;\n    QTimer couchQuitTimer;\n    const auto couchPath=qEnvironmentVariable("PMCOOP_IPC");\n    if (!couchPath.isEmpty()) {\n'
   '        try {couchControl.open(couchPath.toUtf8().toStdString(),false);}\n'
   '        catch(const std::exception& e) {fprintf(stderr,"[COOP] %s\\n",e.what());return 1;}\n'
   '        QObject::connect(&couchQuitTimer,&QTimer::timeout,&melon,[&]{if(couchControl.data->shutdown.load())melon.quit();});\n'
   '        couchQuitTimer.start(50);\n        emuInstances'),
 ],
 'EmuThread.cpp': [
  ('if (getenv("MELONDS_AP")) return;   // automated harness instances: no prompts',
   'if (getenv("PMCOOP_ROLE") || getenv("MELONDS_AP")) return;   // local loopback/harness: no firewall changes'),
  ('if (const char* path = getenv("PMCOOP_IPC")) {\n        try { couch.open(path, false); }',
   'if (const auto path = qEnvironmentVariable("PMCOOP_IPC"); !path.isEmpty()) {\n        try { couch.open(path.toUtf8().toStdString(), false); }'),
 ],
 'EmuInstance.cpp': [
  ('    if (file.empty())\n    {\n        std::string& baseName = gba ? baseGBAAssetName : baseAssetName;',
   '    const char* stableSave = getenv("PMCOOP_SAVE_NAME");\n'
   '    if (!gba && file.empty() && ext == ".sav" && stableSave && !strcmp(stableSave, "game"))\n'
   '    {\n        result += "game";\n    }\n'
   '    else if (file.empty())\n    {\n        std::string& baseName = gba ? baseGBAAssetName : baseAssetName;'),
 ],
}

# Defaults are inserted by getters as well as setters. Protect the whole tree
# while serializing, and each public access while workers/UI run concurrently.
CONFIG_METHODS = (
 'size_t Array::Size()', 'void Array::Clear()', 'Array Array::GetArray(const int id)',
 'int Array::GetInt(const int id)', 'int64_t Array::GetInt64(const int id)',
 'bool Array::GetBool(const int id)', 'std::string Array::GetString(const int id)',
 'double Array::GetDouble(const int id)', 'void Array::SetInt(const int id, int val)',
 'void Array::SetInt64(const int id, int64_t val)', 'void Array::SetBool(const int id, bool val)',
 'void Array::SetString(const int id, const std::string& val)', 'void Array::SetDouble(const int id, double val)',
 'Table& Table::operator=(const Table& b)', 'Array Table::GetArray(const std::string& path)',
 'Table Table::GetTable(const std::string& path, const std::string& defpath)',
 'int Table::GetInt(const std::string& path)', 'int64_t Table::GetInt64(const std::string& path)',
 'bool Table::GetBool(const std::string& path)', 'std::string Table::GetString(const std::string& path)',
 'double Table::GetDouble(const std::string& path)', 'void Table::SetInt(const std::string& path, int val)',
 'void Table::SetInt64(const std::string& path, int64_t val)', 'void Table::SetBool(const std::string& path, bool val)',
 'void Table::SetString(const std::string& path, const std::string& val)',
 'void Table::SetDouble(const std::string& path, double val)',
 'bool Load()', 'void Save()', 'Table GetLocalTable(int instance)',
)
CHANGES['Config.cpp'] = [
 ('#include <regex>', '#include <regex>\n#include <mutex>'),
 ('toml::value RootTable;', 'toml::value RootTable;\nstatic std::recursive_mutex ConfigMutex;'),
 *[(signature+'\n{', signature+'\n{\n    std::lock_guard<std::recursive_mutex> guard(ConfigMutex);') for signature in CONFIG_METHODS],
]

def prepare(source, check=False):
    pending={}
    for name, replacements in CHANGES.items():
        path=source/'src/frontend/qt_sdl'/name
        text=path.read_text(encoding='utf-8')
        for old,new in replacements:
            if new in text: continue
            if check or text.count(old)!=1: raise ValueError('Unexpected upstream edit: '+name)
            text=text.replace(old,new,1)
        pending[path]=text
    if not check:
        for path,text in pending.items(): path.write_text(text,encoding='utf-8',newline='\n')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('--check',action='store_true')
    a=p.parse_args();prepare(a.source,a.check)
