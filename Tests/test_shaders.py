"""Check the generated shader contract consumed by native pipelines."""
import json
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
expected = {
    'MeshDraw': ('meshMain', 'mesh', [1, 1, 1]),
    'TaskDraw': ('taskMain', 'amplification', [1, 1, 1]),
    'DescriptorCompute': ('descriptorMain', 'compute', [64, 1, 1]),
    'IndirectDraw.vs': ('vsMain', 'vertex', None),
    'IndirectDraw.ps': ('psMain', 'fragment', None),
}
if (root / 'Imgui_vs.h').exists():
    expected.update(Imgui_vs=('ImguiVS', 'vertex', None), Imgui_fs=('ImguiFS', 'fragment', None))
if (root / 'NisShaders.h').exists():
    for name in re.findall(r'#include "(NIS_[0-9_]+)\.h"', (root / 'NisShaders.h').read_text()):
        expected[name] = ('NisMain', 'compute', [256, 1, 1])
for name, (entry, stage, threads) in expected.items():
    directory = root if (root / f'{name}.reflection.json').exists() else root / 'Tests'
    reflection = json.loads((directory / f'{name}.reflection.json').read_text())
    function = next(e for e in reflection['entryPoints'] if e['name'] == entry)
    assert function['stage'] == stage, name
    if threads:
        assert function['threadGroupSize'] == threads, name
    binaries = [directory / f'{name}.{ext}' for ext in ('dxil', 'spirv', 'metallib')]
    binary = next(p for p in binaries if p.exists())
    assert binary.read_bytes()[:4] in (b'DXBC', b'\x03\x02\x23\x07', b'MTLB'), name
    header = directory / f'{name}.h'
    if header.exists():
        text = header.read_text()
        assert f'g_{name}_desc' in text and f'"{entry}"' in text, name
    if binary.suffix == '.metallib' and name.startswith(('Imgui_', 'NIS_')):
        parameter = next(p for p in reflection['parameters'] if p['name'] == 'root')
        assert parameter['binding']['index'] == 2, name
        fields = parameter['type']['elementType']['fields']
        offsets = {f['name']: f['binding']['offset'] for f in fields}
        if name.startswith('Imgui_'):
            assert offsets == {'constants': 0, 'samplers': 16, 'textures': 24}, offsets
        else:
            assert offsets == {'constants': 0, 'coefficients': 80, 'samplers': 88, 'images': 96}, offsets
    depfile = (directory / f'{name}.d').read_text()
    assert '.slang' in depfile, name
print(f'{len(expected)} native shader binaries, entry points, layouts and workgroup contracts verified')
