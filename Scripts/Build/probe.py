"""Build bounded CPU qualification programs with native Xmake targets."""
from pathlib import Path
import argparse,json,subprocess,sys,hashlib,shutil
ROOT=Path(__file__).resolve().parents[2]
def through_xmake(command,evidence):
    evidence=Path(evidence).resolve()
    spec=evidence/'xmake-probe.json'
    spec.write_text(json.dumps({'compiler_command':[str(x) for x in command]},indent=2)+'\n')
    return [sys.executable,str(Path(__file__).resolve()),'--spec',str(spec)]
def lua(value):return json.dumps(str(value),ensure_ascii=False)
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--spec',type=Path,required=True)
    args=parser.parse_args();spec=args.spec.resolve();command=json.loads(spec.read_text())['compiler_command']
    from bootstrap import require_xmake
    tool=str(require_xmake());compiler=shutil.which(command[0]) or command[0]
    output=Path(command[command.index('-o')+1]).resolve()
    sources=[];flags=[];index=1
    while index<len(command):
        item=command[index]
        if item=='-o':index+=2;continue
        if Path(item).suffix.lower() in ('.cpp','.cc','.cxx','.c') and not item.startswith('-'):
            sources.append(str(Path(item).resolve()))
        elif not item.startswith('-std='):flags.append(item)
        index+=1
    if not sources:raise ValueError('Probe specification contains no native sources')
    project=spec.parent/'XmakeProbe';project.mkdir(exist_ok=True)
    lines=['set_project("MIRA-CPU-Probe")','set_xmakever("3.1.2")','set_languages("cxx23")',
           'toolchain("probe-clang")','    set_kind("standalone")']
    for kind in ('cc','cxx','ld','sh'):lines.append(f'    set_toolset({lua(kind)}, {lua(compiler)})')
    lines+=['toolchain_end()','set_toolchains("probe-clang")','target("cpu-probe")','    set_kind("binary")',
            f'    set_filename({lua(output.name)})',f'    set_targetdir({lua(output.parent)})',
            f'    set_objectdir({lua(spec.parent/"Build/Objects")})','    set_optimize("none")',
            '    add_files('+', '.join(lua(x) for x in sources)+')']
    if flags:
        raw=', '.join(lua(x) for x in flags)
        lines.append('    add_cxxflags('+raw+', {force = true})')
        # Include paths and preprocessor definitions affect compilation only.
        link_flags=[x for x in flags if not x.startswith(('-I','-D'))]
        if link_flags:lines.append('    add_ldflags('+', '.join(lua(x) for x in link_flags)+', {force = true})')
    lines+=['target_end()']
    (project/'xmake.lua').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    receipts=[]
    for argv in ([tool,'f','-P',str(project),'-y'],[tool,'build','-P',str(project),'-y','cpu-probe']):
        result=subprocess.run(argv,text=True,capture_output=True,cwd=project)
        print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr)
        receipts.append({'argv':argv,'exit_code':result.returncode})
        if result.returncode:break
    receipt={'result':'passed' if not result.returncode else 'failed','commands':receipts,
             'spec_sha256':hashlib.sha256(spec.read_bytes()).hexdigest(),
             'xmake_project_sha256':hashlib.sha256((project/'xmake.lua').read_bytes()).hexdigest(),
             'compiler':compiler,'xmake':tool}
    (spec.parent/'xmake-build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return result.returncode
if __name__=='__main__':raise SystemExit(main())
