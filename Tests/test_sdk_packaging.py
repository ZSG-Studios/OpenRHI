"""Validate SDK packaging through the public Xmake task using isolated fixture inputs."""
from pathlib import Path
import argparse,json,os,shutil,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Scripts/Build'))
from bootstrap import require_xmake
RUNTIMES=('amd_ags_x64.dll','amd_fidelityfx_loader_dx12.dll','amd_fidelityfx_upscaler_dx12.dll','amd_fidelityfx_denoiser_dx12.dll','amd_fidelityfx_radiancecache_dx12.dll','amd_fidelityfx_framegeneration_dx12.dll','NRC_D3D12.dll','NRC_Vulkan.dll','cudart64_13.dll','nvrtc64_130_0.dll','nvrtc-builtins64_134.dll','amd_fidelityfx_vk.dll','nvngx_dlss.dll','nvngx_dlssd.dll','libxess.dll','libanf.so')
def run(root,output,optional):
    fixture=Path(tempfile.mkdtemp(prefix='sdk-fixture-',dir=output))
    for directory in ('Include','Xmake','Scripts/Build','Docs'):shutil.copytree(root/directory,fixture/directory)
    for name in ('LICENSE.txt','THIRD_PARTY_NOTICES.md','README.md','openrhi.natvis'):shutil.copy2(root/name,fixture/name)
    (fixture/'xmake.lua').write_text('set_project("SDK-packaging-fixture")\nset_xmakever("3.1.2")\ntask("sdk")\nset_category("plugin")\nset_menu {usage="xmake sdk", options={}}\non_run(function() os.execv("python", {path.join(os.projectdir(),"Scripts/Build/package.py"), "--output", path.join(os.projectdir(),"_OpenRHI_SDK")}) end)\ntask_end()\n')
    sources=[]
    for mode in ('debug','release'):
        names=['OpenRHI.dll','OpenRHI.lib','AgilitySDK/D3D12Core.dll','ThirdPartyLicenses/vendor/LICENSE.txt']
        if optional:names.extend(RUNTIMES)
        for name in names:
            source=fixture/'Bin/windows/x64'/mode/name;source.parent.mkdir(parents=True,exist_ok=True);source.write_bytes((mode+'/'+name).encode())
            sources.append((source,fixture/'_OpenRHI_SDK/Lib'/mode.title()/name))
    process=subprocess.run([str(require_xmake()),'sdk','-P',str(fixture)],cwd=fixture,capture_output=True,text=True)
    (fixture/'package.log').write_text(process.stdout+process.stderr);process.check_returncode()
    for source,packaged in sources:assert source.read_bytes()==packaged.read_bytes(),str(packaged)
    sdk=fixture/'_OpenRHI_SDK'
    for name in ('Xmake/openrhi_slang.lua','Scripts/Build/shader.py','Scripts/Build/dependencies.py','Include/OpenRHI.h'):
        assert (sdk/name).read_bytes()==(root/name).read_bytes(),name
    assert not (sdk/'CMake').exists()
    assert not any(p.is_symlink() or p.is_junction() for p in sdk.rglob('*'))
    print('Xmake SDK optional='+str(optional)+': PASS')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,default=ROOT);p.add_argument('--output',type=Path,required=True);p.add_argument('--bash')
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    for optional in (False,True):run(args.root.resolve(),args.output.resolve(),optional)
