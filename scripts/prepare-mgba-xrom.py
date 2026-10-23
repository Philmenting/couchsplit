#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build an owned patched copy of pinned mGBA; never edit the upstream checkout."""
from pathlib import Path
import hashlib,json,shutil,subprocess,sys

def prepare(source,output):
 source=Path(source).resolve();output=Path(output).resolve();root=Path(__file__).resolve().parents[1]
 if source==output or source in output.parents or output in source.parents:raise ValueError('Source and output must be separate directories')
 lock=json.loads((root/'config/mgba-xrom.lock.json').read_text());patch=root/'patches/mgba-xrom.patch'
 for name,h in lock['files'].items():
  if hashlib.sha256((source/name).read_bytes()).hexdigest()!=h:raise ValueError('Unexpected mGBA input: '+name)
 stamp=output/'.couchsplit-xrom.json';identity={'upstream':lock['upstream_commit'],'patch':hashlib.sha256(patch.read_bytes()).hexdigest()}
 if stamp.exists():
  if json.loads(stamp.read_text())!=identity:raise ValueError('Use a fresh output directory for changed XROM patch')
 else:
  if output.exists() and any(output.iterdir()):raise ValueError('Output must be empty or a previously prepared XROM copy')
  shutil.copytree(source,output,ignore=shutil.ignore_patterns('.git','__pycache__'),dirs_exist_ok=True)
  subprocess.run(['git','apply','--check',str(patch)],cwd=output,check=True)
  subprocess.run(['git','apply',str(patch)],cwd=output,check=True)
  stamp.write_text(json.dumps(identity,indent=2)+'\n')
 for name,h in lock['patched_files'].items():
  if hashlib.sha256((output/name).read_bytes()).hexdigest()!=h:raise ValueError('Patched mGBA differs: '+name)
 return output
if __name__=='__main__':
 try:print(prepare(*sys.argv[1:]))
 except (OSError,ValueError,subprocess.CalledProcessError) as e:sys.exit(str(e))
