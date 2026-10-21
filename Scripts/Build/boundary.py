"""Check owned native API, shader and Xmake source boundaries."""
from pathlib import Path
import argparse,re

def audit(root):
    root=Path(root).resolve();provenance={'THIRD_PARTY_NOTICES.md','Docs/UPSTREAM.md'}
    legacy_api=r'[Dd][Xx]'+r'11|[Dd]3[Dd]'+r'11|[Dd]irect3[Dd]'+r'[ -]?11'
    translation=r'[Mm]olten'+r'[Vv][Kk]|[Pp]ortability_[Ee]numeration|[Pp]ortability_[Ss]ubset|[Mm]etal_[Ss]urface|VK_USE_PLATFORM_'+r'METAL'
    browser=r'[Ww]eb'+r'[Gg][Pp][Uu]|[Ww][Gg]'+r'[Pp][Uu]|[Ww][Gg]'+r'[Ss][Ll]'
    old_name=r'(^|[^A-Za-z0-9])([Nn][Rr][Ii]'+r'([A-Z_][A-Za-z0-9_]*|[^A-Za-z0-9_]|$)|[Aa]rc[Rr]ender[A-Za-z0-9_]*)'
    wrong_brand='OPEN'+'RHI[A-Za-z]'
    texture_url='https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D'+'11_3_FunctionalSpec.htm'
    files=[]
    for directory in ('Include','Source','Scripts','Xmake','Shaders','Tests','Profiles','Docs','.github','.agents'):
        files.extend(p for p in (root/directory).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    files.extend(root/name for name in ('xmake.lua','README.md','AGENTS.md','openrhi.natvis'))
    violations=[]
    for path in files:
        if path.suffix.lower() not in ('.h','.hpp','.cpp','.c','.lua','.md','.txt','.json','.py','.ps1','.bat','.sh','.slang','.yml','.yaml','.natvis','.toml','.zig','.rs','.lock'):continue
        relative=path.relative_to(root).as_posix()
        if relative in provenance or 'ThirdPartyLicenses' in path.parts:continue
        content=path.read_text(encoding='utf-8-sig',errors='replace')
        checked=content.replace(texture_url,'')
        if re.search(legacy_api+'|'+translation+'|'+browser,relative+'\n'+checked):violations.append('unsupported backend '+relative)
        if re.search(old_name+'|'+wrong_brand,relative+'\n'+content):violations.append('public API identity '+relative)
        if relative.startswith('Source/VK/') and re.search(r'WIN32|_WIN32|Win32|vulkan-1.dll',content):violations.append('Windows Vulkan source '+relative)
    if violations:raise RuntimeError('\n'.join(violations))
    print('OpenRHI source boundary passed:',len(files),'files')
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[2]);args=parser.parse_args();audit(args.root)
