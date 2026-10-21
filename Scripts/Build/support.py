"""Xmake build hooks for source admission, generated shaders and runtime staging."""
from pathlib import Path
import argparse,json,os,platform,shutil,subprocess
from dependencies import ROOT,require,sha
from shader import compile_shader,parser as shader_parser

def selected_dependencies(features):
    result=[]
    for feature,name in {'d3d12':'d3d12ma','agility':'agilitysdk','nvtx':'nvtx','vk':'vulkan_headers','metal':'metal_cpp','nvapi':'nvapi','amdags':'amdags','ngx':'ngx','nrc':'nrc','antilag2':'antilag2','xess':'xess'}.items():
        if feature in features:result.append(name)
    if 'vk' in features:result.append('vma')
    if 'ffx' in features:result.append('fsr' if 'd3d12' in features else 'ffx')
    return result

def prepare(args):
    features=set(filter(None,args.features.split(',')));generated=Path(args.generated);generated.mkdir(parents=True,exist_ok=True)
    for name in selected_dependencies(features):
        override={'vulkan_headers':'OPENRHI_VULKAN_HEADERS_ROOT','vma':'OPENRHI_VMA_ROOT','nvapi':'OPENRHI_NVAPI_ROOT'}.get(name)
        if not(override and os.environ.get(override)):require(name)
    if 'anf' in features:
        anf=Path(args.anf_root)
        for filename in ('public/anf.h','public/anf_types.h','public/anf_types_vk.h','public/anf_sr.h','public/anf_fg.h','lib/arm64-v8a/libanf.so','LICENSE.txt','LICENSE-BSD-3-Clause.txt'):
            if not (anf/filename).is_file():raise RuntimeError('Missing ANF SDK input: '+filename)
        if sha(anf/'lib/arm64-v8a/libanf.so')!='284e8a1e0ec0a572eb56b1d06c778def9db0cd09e4ea20564caaa9612089384c':raise RuntimeError('ANF SDK runtime differs from admitted0.43.0')
    if 'agility' in features:
        (generated/'OpenRHIAgilitySDK.h').write_text('#pragma once\n#include <stdint.h>\n#ifdef __cplusplus\nextern "C" {\n#endif\n__declspec(dllexport) extern const uint32_t D3D12SDKVersion;\nconst uint32_t D3D12SDKVersion = 619;\n__declspec(dllexport) extern const char* D3D12SDKPath;\nconst char* D3D12SDKPath = "AgilitySDK/";\n#ifdef __cplusplus\n}\n#endif\n')
    target='dxil' if 'd3d12' in features else ('metal' if 'metal' in features else 'spirv')
    extension={'dxil':'dxil','spirv':'spirv','metal':'metallib' if platform.system()=='Darwin' else 'metal'}[target]
    def shader(name,source,entry=None,stage='compute',defines=(),header=True):
        arguments=['--source',str(ROOT/source),'--target',target,'--output',str(generated/(name+'.'+extension)),'--include',str(ROOT/'Include'),'--include',str(ROOT/'Shaders'),'--apple-sdk',args.apple_sdk]
        if entry:arguments+=['--entry',entry,'--stage',stage]
        if header:arguments+=['--header',str(generated/(name+'.h')),'--symbol',name]
        for definition in defines:arguments+=['--define',definition]
        compile_shader(shader_parser().parse_args(arguments))
    for feature,prefix in (('imgui','Imgui'),('rmlui','RmlUi')):
        if feature in features:
            for suffix,entry,stage in (('vs',prefix+'VS','vertex'),('fs',prefix+'FS','fragment')):
                shader(prefix+'_'+suffix,'Shaders/'+prefix+'.slang',entry,stage)
    if 'nis' in features:
        includes=[];table=[]
        for half in range(2):
            for hdr in range(2):
                for output_format in range(4):
                    actual=output_format if 'vk' in features else 0
                    name=f'NIS_{half}_{hdr}_{actual}'
                    if actual==output_format:
                        shader(name,'Shaders/NIS.slang','NisMain',defines=(f'NIS_FP16={half}',f'NIS_HDR_MODE={hdr}',f'NIS_OUTPUT_FORMAT={actual}','NIS_THREAD_GROUP_SIZE=256'))
                        includes.append(f'#include "{name}.h"\n')
                    table.append(f'    g_{name}_desc,\n')
        (generated/'NisShaders.h').write_text(''.join(includes)+'static const ShaderDesc g_NisShaders[] = {\n'+''.join(table)+'};\n')
    if 'metal' in features:
        groups=['CLEAR','BUFFER','RESOLVE_VS','FILTER','ROOTS','DEPTH','INSTANCES','HEADER','WORDS']+[f'STORAGE_{d}_{t}' for d in range(6) for t in range(3)]+[f'RESOLVE_{d}_{t}' for d in ('2d','array') for t in range(3)]
        for group in groups:shader('Internal_'+group,'Shaders/InternalMetal.slang',defines=('OPENRHI_INTERNAL_'+group+'=1',),header=False)
        output=generated/'InternalMetal.metallib'
        subprocess.run(['xcrun','-sdk',args.apple_sdk,'metallib',*[str(generated/('Internal_'+g+'.air')) for g in groups],'-o',str(output)],check=True)
        payload=','.join('0x%02x'%b for b in output.read_bytes())
        (generated/'InternalMetal.metallib.h').write_text('#pragma once\nalignas(8) static const unsigned char g_InternalMetal_metallib[] = {'+payload+'};\n')
    (generated/'admission.json').write_text(json.dumps({'features':sorted(features),'platform':args.platform,'arch':args.arch,'mode':args.mode,'builder':'Xmake 3.1.2'},indent=2)+'\n')

def runtime(args):
    features=set(filter(None,args.features.split(',')));output=Path(args.output);output.mkdir(parents=True,exist_ok=True)
    arch='arm64' if args.arch in ('arm64','arm64-v8a') else 'x64'
    def copy(path,destination=None):
        path=Path(path)
        if not path.is_file():raise RuntimeError('Missing admitted runtime input: '+str(path))
        target=output/(destination or path.name);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,target)
    for name in selected_dependencies(features):
        source=require(name)
        for path in source.iterdir():
            if path.is_file() and any(word in path.name.lower() for word in ('license','copying','notice')):copy(path,'ThirdPartyLicenses/'+name+'/'+path.name)
    if 'agility' in features:
        source=require('agilitysdk')
        shutil.copytree(source/'build/native/bin'/arch,output/'AgilitySDK',dirs_exist_ok=True)
        for name in ('LICENSE.txt','LICENSE-CODE.txt'):copy(source/name,'AgilitySDK/'+name)
    if 'amdags' in features:copy(require('amdags')/'ags_lib/lib'/('amd_ags_'+arch+'.dll'))
    if 'ffx' in features and 'd3d12' in features:
        source=require('fsr');effects=['loader','upscaler','denoiser','radiancecache']
        if 'frame_generation' in features:effects.append('framegeneration')
        for effect in effects:copy(source/'Kits/FidelityFX/signedbin'/('amd_fidelityfx_'+effect+'_dx12.dll'))
        copy(source/'Kits/FidelityFX/docs/license.md','ThirdPartyLicenses/fsr/LICENSE.md')
        copy(source/'3rdpartynotice.md','ThirdPartyLicenses/fsr/3rdpartynotice.md')
    if 'ngx' in features:
        source=require('ngx');mode='dev' if args.mode=='debug' else 'rel'
        if args.platform=='windows':
            directory='Windows_aarch64' if arch=='arm64' else 'Windows_x86_64'
            for name in ('nvngx_dlss.dll','nvngx_dlssd.dll'):copy(source/'lib'/directory/mode/name)
        else:
            for name in ('dlss','dlssd'):copy(source/'lib/Linux_x86_64'/mode/('libnvidia-ngx-'+name+'.so.310.9.16'))
    if 'nrc' in features:
        for name in ('NRC_D3D12.dll','cudart64_13.dll','nvrtc64_130_0.dll','nvrtc-builtins64_134.dll'):copy(require('nrc')/'Bin'/arch/name)
    if 'xess' in features:copy(require('xess')/'bin/libxess.dll')
    if 'anf' in features:
        source=Path(args.anf_root);copy(source/'lib/arm64-v8a/libanf.so')
        for name in ('LICENSE.txt','LICENSE-BSD-3-Clause.txt'):copy(source/name,'ThirdPartyLicenses/anf/'+name)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('action',choices=('prepare','runtime'))
    for name in ('features','generated','output','platform','arch','mode','anf-root','apple-sdk'):parser.add_argument('--'+name,default='')
    args=parser.parse_args();globals()[args.action](args)
