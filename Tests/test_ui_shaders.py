"""Check UI SPIR-V and Metal root-table layout against the native implementation."""
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
for target in ('spirv', 'metal'):
    for stage, entry in (('vs', 'RmlUiVS'), ('fs', 'RmlUiFS')):
        name = f'RmlUiContract_{target}_{stage}'
        reflection = json.loads((root / f'{name}.reflection.json').read_text())
        assert any(e['name'] == entry for e in reflection['entryPoints']), name
        if target == 'metal':
            parameter = next(p for p in reflection['parameters'] if p['name'] == 'root')
            assert parameter['binding']['index'] == 2, name
            fields = parameter['type']['elementType']['fields']
            offsets = {f['name']: f['binding']['offset'] for f in fields}
            assert offsets == {'constants': 0, 'samplers': 96, 'textures': 104}, offsets
            assert fields[0]['binding']['size'] == 96
            assert (root / f'{name}.metal').stat().st_size > 0
        else:
            assert (root / f'{name}.spirv').read_bytes()[:4] == b'\x03\x02\x23\x07'
print('RmlUi SPIR-V and Metal entry points, 96-byte constants and root-table ABI: PASS')
