"""Stage the Xmake-built SDK, public headers, shader helpers and runtime licenses."""
from pathlib import Path
import argparse,hashlib,json,os,shutil
from dependencies import ROOT,require,slang_host

def package(root,output,with_compiler=False):
    root=Path(root).resolve();output=Path(output).resolve();output.mkdir(parents=True,exist_ok=True)
    for directory in ('Include','Docs','Xmake','Scripts/Build'):
        shutil.copytree(root/directory,output/directory,dirs_exist_ok=True,ignore=shutil.ignore_patterns("__pycache__","*.pyc"))
    for filename in ('LICENSE.txt','THIRD_PARTY_NOTICES.md','README.md','openrhi.natvis'):
        shutil.copy2(root/filename,output/filename)
    platforms={str(p.parent.relative_to(root/'Bin')) for p in (root/'Bin').glob('*/*/*') if p.is_dir() and p.name in ('debug','release')}
    if len(platforms)>1:raise RuntimeError('Package one native platform/architecture per SDK root: '+', '.join(sorted(platforms)))
    for source in (root/'Bin').glob('*/*/*'):
        if not source.is_dir():continue
        mode=source.name.title();platform=source.parent.parent.name;arch=source.parent.name
        if mode not in ('Debug','Release'):continue
        # One platform/architecture per SDK package; require distinct SDK roots for cross builds.
        destination=output/'Lib'/mode
        shutil.copytree(source,destination,dirs_exist_ok=True)
    generated=root/'Build/Generated'
    for header in generated.glob('*/*/*/OpenRHIAgilitySDK.h'):
        shutil.copy2(header,output/'Include/OpenRHIAgilitySDK.h')
    (output/'xmake.lua').write_text('set_project("OpenRHI-SDK")\nset_xmakever("3.1.2")\nincludes("Xmake/tasks.lua")\n')
    if with_compiler:
        shutil.copytree(require('slang-'+slang_host(),root),output/'Tools/Slang',dirs_exist_ok=True)
        if os.name=='nt':shutil.copytree(require('slang_dxil',root),output/'Tools/Slang/DXIL',dirs_exist_ok=True)
    hashes={str(p.relative_to(output)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in output.rglob('*') if p.is_file() and p.name!='sdk-inputs.json'}
    (output/'sdk-inputs.json').write_text(json.dumps({'builder':'Xmake 3.1.2','files':hashes},indent=2)+'\n')
    return output
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--with-compiler',action='store_true')
    args=parser.parse_args();print(package(ROOT,args.output,args.with_compiler))
