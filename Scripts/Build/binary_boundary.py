"""Audit exports and runtime imports through the native Windows binary inspector."""
from pathlib import Path
import argparse,re,shutil,subprocess
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--library',type=Path,required=True);p.add_argument('--dumpbin',default='dumpbin');p.add_argument('--d3d12',action='store_true');args=p.parse_args()
exports=subprocess.check_output([args.dumpbin,'/exports',str(args.library)],text=True)
old_prefix=r'[ \t]n'+'ri[A-Z]';wrong_brand='OPEN'+'RHI[A-Za-z]'
assert not re.search('openrhiCreateDeviceFromVKDevice|openrhiCreateDeviceFromMetalDevice|'+old_prefix+'|'+wrong_brand,exports)
for name in ('openrhiGetDeviceProfile','openrhiConvertOpenRHIFormatToDXGI','openrhiConvertOpenRHIFormatToVK'):assert name in exports,name
assert ('openrhiCreateDeviceFromD3D12Device' in exports)==args.d3d12
imports=subprocess.check_output([args.dumpbin,'/dependents',str(args.library)],text=True).lower()
assert not re.search('vulkan|slang|dxcompiler|dxil',imports)
print('OpenRHI Windows native boundary passed')
