"""Compile pinned Slang shaders for an Xmake rule or task."""
from pathlib import Path
import argparse,json,os,platform,re,shutil,subprocess
from dependencies import require,slang_host
ROOT=Path(__file__).resolve().parents[2]
def compile_shader(args):
    source=Path(args.source).resolve();output=Path(args.output).resolve();output.parent.mkdir(parents=True,exist_ok=True)
    if source.suffix!='.slang':raise ValueError('Slang source required')
    slang=Path(args.slang_root or os.environ.get('OPENRHI_SLANG_ROOT') or (ROOT/'Tools/Slang' if (ROOT/'Tools/Slang/bin').is_dir() else require('slang-'+slang_host())))
    compiler=slang/'bin'/('slangc.exe' if platform.system()=='Windows' else 'slangc')
    version=subprocess.run([str(compiler),'-version'],text=True,capture_output=True,check=True)
    if (version.stdout+version.stderr).strip()!='2026.20':raise RuntimeError('OpenRHI requires Slang 2026.20')
    reflection=Path(args.reflection).resolve() if args.reflection else output.with_suffix('.reflection.json')
    reflection.parent.mkdir(parents=True,exist_ok=True)
    command=[str(compiler),str(source),'-lang','slang','-target',args.target,'-O3','-warnings-as-errors','all','-reflection-json',str(reflection),'-depfile',str(output)+'.d']
    if args.entry:command+=['-entry',args.entry,'-stage',args.stage]
    for item in args.include:command+=['-I',str(Path(item).resolve())]
    for item in args.define:command+=['-D'+item]
    if args.target=='dxil':
        if platform.system()!='Windows':raise RuntimeError('Pinned DXIL compiler requires Windows host')
        downstream=Path(args.dxil_root or os.environ.get('OPENRHI_SLANG_DXIL_ROOT') or (ROOT/'Tools/Slang/DXIL' if (ROOT/'Tools/Slang/DXIL/bin').is_dir() else require('slang_dxil')))
        arch='arm64' if platform.machine().lower() in ('arm64','aarch64') else 'x64'
        command+=['-profile','sm_6_6','-dxc-path',str(downstream/'bin'/arch),'-DOPENRHI_TARGET_D3D12=1']
    elif args.target=='spirv':
        command+=['-profile','sm_6_6','-capability','spirv_1_6','-fvk-use-entrypoint-name','-DOPENRHI_TARGET_VULKAN=1']
        if args.stage in ('mesh','amplification'):command+=['-capability','spvMeshShadingEXT']
    else:command+=['-capability','metallib_4_0','-line-directive-mode','none','-DOPENRHI_TARGET_METAL=1']
    binary=output
    if args.target=='metal' and platform.system()=='Darwin':binary=output.with_suffix('.metal')
    command+=['-o',str(binary)]
    subprocess.run(command,check=True)
    if args.target=='metal' and platform.system()=='Darwin':
        sdk=args.apple_sdk;minimum={'macosx':'-mmacosx-version-min=26.0','iphoneos':'-mios-version-min=26.0','iphonesimulator':'-mios-simulator-version-min=26.0'}[sdk]
        air=output.with_suffix('.air')
        subprocess.run(['xcrun','-sdk',sdk,'metal','-std=metal4.0',minimum,'-c',str(binary),'-o',str(air)],check=True)
        subprocess.run(['xcrun','-sdk',sdk,'metallib',str(air),'-o',str(output)],check=True)
    if args.header:
        symbol=args.symbol or output.stem
        if not re.fullmatch('[A-Za-z_][A-Za-z0-9_]*',symbol):raise ValueError('Header symbol must be a C identifier')
        if args.target=='metal' and platform.system()!='Darwin':raise RuntimeError('Metallib headers require Apple assembler host')
        payload=','.join('0x%02x'%b for b in output.read_bytes())
        text=f'#pragma once\nalignas(8) static const unsigned char g_{symbol}[] = {{{payload}}};\n'
        if args.entry:
            entry=next((e for e in json.loads(reflection.read_text())['entryPoints'] if e['name']==args.entry),None)
            if entry is None:raise ValueError('Shader entry absent from reflection')
            stage=entry['stage'];threads=entry.get('threadGroupSize',[1,1,1]) if stage in ('compute','mesh','amplification') else [1,1,1]
            if len(threads)!=3 or any(not 1<=t<=65535 for t in threads):raise ValueError('Invalid reflected threadgroup dimensions')
            name={'amplification':'TASK','hull':'TESS_CONTROL','domain':'TESS_EVALUATION','raygeneration':'RAYGEN','closesthit':'CLOSEST_HIT','anyhit':'ANY_HIT'}.get(stage,stage.upper())
            text+=f'static constexpr const char* g_{symbol}_entry = "{args.entry}";\nstatic constexpr uint32_t g_{symbol}_threads[3] = {{{threads[0]}, {threads[1]}, {threads[2]}}};\n'
            text+=f'static const openrhi::ShaderDesc g_{symbol}_desc = {{openrhi::StageBits::{name}_SHADER, g_{symbol}, sizeof(g_{symbol}), g_{symbol}_entry, openrhi::Dim_t(g_{symbol}_threads[0]), openrhi::Dim_t(g_{symbol}_threads[1]), openrhi::Dim_t(g_{symbol}_threads[2]), {args.task_payload_size}}};\n'
        Path(args.header).parent.mkdir(parents=True,exist_ok=True);Path(args.header).write_text(text)
    return output
def parser():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',required=True);p.add_argument('--entry');p.add_argument('--stage',default='compute')
    p.add_argument('--target',choices=('dxil','spirv','metal'),required=True);p.add_argument('--output',required=True)
    p.add_argument('--include',action='append',default=[]);p.add_argument('--define',action='append',default=[])
    p.add_argument('--slang-root');p.add_argument('--dxil-root');p.add_argument('--reflection');p.add_argument('--header');p.add_argument('--symbol','--name',dest='symbol')
    p.add_argument('--task-payload-size',type=int,default=0);p.add_argument('--apple-sdk',choices=('macosx','iphoneos','iphonesimulator'),default='macosx')
    return p
if __name__=='__main__':compile_shader(parser().parse_args())
