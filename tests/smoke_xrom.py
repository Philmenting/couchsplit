#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Authored ARM fixture: real CPU, DMA and BIOS reads beyond 32 MiB, no game assets."""
from pathlib import Path
import os,sys,json,struct,subprocess,mmap,tempfile,time

def fixture(path,size=64*1024*1024):
 code=[];lit=[];loads=[];branches=[];labels={}
 def emit(x):code.append(x)
 def load(r,x):loads.append((len(code),r,len(lit)));emit(0);lit.append(x)
 def branch(label,cond=14):branches.append((len(code),label,cond));emit(0)
 def eq(reg,value):load(10,len(branches));load(9,value);emit(0xE1500009|reg<<16);branch('fail',1)
 def word(addr,value):load(0,addr);load(1,value);emit(0xE5801000)
 def verify(addr,value):load(0,addr);emit(0xE5901000);eq(1,value)
 base=0x10000000
 load(0,base);emit(0xE5D01003);eq(1,0x44) # LDRB
 emit(0xE1D010B2);eq(1,0x4433) # LDRH
 emit(0xE5901000);eq(1,0x44332211) # LDR
 emit(0xE5901001);eq(1,0x11443322) # unaligned rotation
 emit(0xE8B00006);eq(1,0x44332211);eq(2,0x88776655) # LDMIA
 # Near the end of the actual requested file capacity.
 load(0,base+size-32*1024*1024-4);emit(0xE5901000);eq(1,0x89ABCDEF)
 # DMA3 in both transfer widths.
 for ctrl,dest in [(0x80000004,0x02000100),(0x84000002,0x02000110)]:
  word(0x040000D4,base);word(0x040000D8,dest);word(0x040000DC,ctrl);verify(dest,0x44332211);verify(dest+4,0x88776655)
 # BIOS CpuSet and LZ77 WRAM/VRAM, sourced from extended data.
 load(0,base);load(1,0x02000200);load(2,0x04000002);emit(0xEF0B0000);verify(0x02000200,0x44332211)
 for dest,swi in [(0x02000300,0x11),(0x06000000,0x12)]:
  load(0,base+0x40);load(1,dest);emit(0xEF000000|(swi<<16));verify(dest,0x44434241);verify(dest+4,0x48474645)
 # Save markers distinguish players after isolated A input and survive cold restart.
 load(6,0x0E000000);emit(0xE5D67000);emit(0xE357005A);emit(0x13A07020);emit(0xE5C67000)
 load(8,0x03E0);branch('display')
 labels['fail']=len(code);load(0,0x02000010);emit(0xE5801000);emit(0xE5809004);emit(0xE580A008);load(8,0x1F);emit(0xE1A0728A)
 labels['display']=len(code);word(0x04000000,0x403);load(5,0x04000130)
 labels['loop']=len(code);load(4,0x06000000);load(3,240*160);emit(0xE188A007)
 labels['draw']=len(code);emit(0xE0C4A0B2);emit(0xE2533001);branch('draw',1)
 emit(0xE1D510B0);emit(0xE3110001);branch('loop',1);emit(0xE3A0705A);emit(0xE5C67000);branch('loop')
 for i,r,slot in loads:
  delta=4*(len(code)+slot-i)-8;assert 0<=delta<4096;code[i]=0xE59F0000|r<<12|delta
 for i,label,cond in branches:code[i]=cond<<28|0x0A000000|((labels[label]-i-2)&0xFFFFFF)
 header=bytearray(0x1000);struct.pack_into('<I',header,0,0xEA00007E);header[0xA0:0xAC]=b'XROM TEST   ';header[0xAC:0xB0]=b'CSXT';header[0xB2]=0x96;header[0xB5:0xBC]=b'CSXROM1';header[0xBD]=(-sum(header[0xA0:0xBD])-0x19)&255
 data=struct.pack('<'+'I'*(len(code)+len(lit)),*(code+lit));header[0x200:0x200+len(data)]=data;header[0xF00:0xF09]=b'SRAM_V113'
 with Path(path).open('wb') as f:
  f.write(header);f.truncate(size);f.seek(32*1024*1024);f.write(bytes.fromhex('1122334455667788'));f.seek(32*1024*1024+0x40);f.write(bytes.fromhex('1008000000')+b'ABCDEFGH'+bytes(3));f.seek(size-4);f.write(struct.pack('<I',0x89ABCDEF))

def run(binary,output=None):
 binary=Path(binary).resolve();assert b'GBA_XROM=1' in subprocess.check_output([str(binary),'--coop-capabilities'])
 results=[]
 with tempfile.TemporaryDirectory(prefix='couch-xrom-') as temp:
  root=Path(temp)
  for mib in [64,512]:
   rom=root/f'test{mib}.gba';fixture(rom,mib*1024*1024)
   profiles=[root/f'{mib}-{i}' for i in range(2)]
   for p in profiles:(p/'saves').mkdir(parents=True)
   for restart in [False,True]:
    children=[];files=[];maps=[];logs=[]
    try:
     for p in profiles:
      ipc=p/'control.ipc';ipc.write_bytes(struct.pack('<10I',0x434F5031,2,0,0xFFF,0,0,0,0,0,0)+bytes(256*192*4*2));f=ipc.open('r+b');files.append(f);maps.append(mmap.mmap(f.fileno(),0));log=(p/f'{restart}.log').open('wb');logs.append(log)
      env={k:v for k,v in os.environ.items() if not k.startswith(('PMCOOP_','MELONDS_AP'))};env.update(PMCOOP_IPC=str(ipc),PMCOOP_PROFILE=str(p),SDL_AUDIODRIVER='dummy');children.append(subprocess.Popen([str(binary),str(rom)],env=env,stdout=log,stderr=subprocess.STDOUT))
     def pixel(i):return struct.unpack_from('<I',maps[i],40)[0]
     def wait(pred):
      deadline=time.monotonic()+15
      while time.monotonic()<deadline:
       assert all(c.poll() is None for c in children),'worker exited'
       if pred():return
       time.sleep(.03)
      raise AssertionError(('XROM checks failed',mib,restart,[hex(pixel(i)) for i in range(2)]))
     wait(lambda:all(struct.unpack_from('<I',m,20)[0]>10 for m in maps))
     # Green component means every authored CPU/DMA/BIOS check reached success.
     assert all(pixel(i)&0x00FF00==0x00FF00 for i in range(2)),[hex(pixel(i)) for i in range(2)]
     if restart:assert pixel(0)!=pixel(1),'cold save reload'
     else:
      initial=pixel(0);assert initial==pixel(1);struct.pack_into('<I',maps[0],12,0xFFE);wait(lambda:pixel(0)!=initial and pixel(1)==initial);struct.pack_into('<I',maps[0],12,0xFFF)
     for m in maps:struct.pack_into('<I',m,36,1)
     for c in children:assert c.wait(timeout=5)==0
    except Exception:
     for log in logs:log.flush()
     for p in profiles:
      for f in p.glob('*.log'):print(f.read_text(errors='replace')[-4000:])
     raise
    finally:
     for c in children:
      if c.poll() is None:c.terminate();c.wait(timeout=5)
     for m in maps:m.close()
     for f in files:f.close()
     for f in logs:f.close()
   assert [(p/'saves/game.sav').read_bytes()[0] for p in profiles]==[0x5A,0x20]
   results.append({'rom_mib':mib,'cpu_8_16_32_unaligned_ldm':True,'dma_16_32':True,'bios_cpuset_lz77_wram_vram':True,'tail_access':True,'input_isolation_save_reload':True})
 if output:Path(output).write_text(json.dumps({'production_worker':True,'authored_fixture_only':True,'checks':results},indent=2)+'\n')
 print('PASS XROM 64/512 MiB CPU, DMA, BIOS, capacity, isolated input, save/reload')
if __name__=='__main__':run(*sys.argv[1:])
