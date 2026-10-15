#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply the small, idempotent ABI-2 additions after the pinned base patch."""
import argparse
from pathlib import Path

CHANGES = {
 'main.cpp': [
  ('#include <QTimer>', '#include <QTimer>\n#include "CoopIPC.h"'),
  ('if (const char* profile = getenv("PMCOOP_PROFILE")) {\n        emuDirectory = QString::fromUtf8(profile);',
   'if (const auto profile = qEnvironmentVariable("PMCOOP_PROFILE"); !profile.isEmpty()) {\n        emuDirectory = profile;'),
  ('PMCOOP_ABI=1;BRIDGE_WIRE=2;POSIX=1','PMCOOP_ABI=2;BRIDGE_WIRE=2'),
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
  ('if (const char* path = getenv("PMCOOP_IPC")) {\n        try { couch.open(path, false); }',
   'if (const auto path = qEnvironmentVariable("PMCOOP_IPC"); !path.isEmpty()) {\n        try { couch.open(path.toUtf8().toStdString(), false); }'),
 ],
}
def prepare(source, check=False):
    pending={}
    for name, replacements in CHANGES.items():
        path=source/'src/frontend/qt_sdl'/name
        text=path.read_text()
        for old,new in replacements:
            if new in text: continue
            if check or text.count(old)!=1: raise ValueError('Unexpected upstream edit: '+name)
            text=text.replace(old,new,1)
        pending[path]=text
    if not check:
        for path,text in pending.items(): path.write_text(text)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('--check',action='store_true')
    a=p.parse_args();prepare(a.source,a.check)
