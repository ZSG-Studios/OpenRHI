"""CPU-only pinned compiler contract for independent nonuniform SRV/UAV heap indices."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--dxc", type=Path, required=True)
parser.add_argument("--slang", type=Path, required=True)
parser.add_argument("--source", type=Path, required=True)
parser.add_argument("--driver", type=Path, required=True)
parser.add_argument("--stored", type=Path, required=True)
parser.add_argument("--direct", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def inspect(label, binary):
    result = subprocess.run([str(args.dxc), "-dumpbin", str(binary)], capture_output=True,
                            text=True, errors="strict", check=True)
    (args.output / f"{label}.dxil.txt").write_text(result.stdout)
    reflection = json.loads(binary.with_suffix(".reflection.json").read_text())
    function = next(entry for entry in reflection["entryPoints"] if entry["name"] == "nonUniformIndexMain")
    assert function["stage"] == "compute" and function["threadGroupSize"] == [64, 1, 1], label
    parameter = next(value for value in reflection["parameters"] if value["name"] == "HeapIndices")
    assert parameter["binding"] == {"kind": "constantBuffer", "space": 1, "index": 0}, label
    fields = parameter["type"]["elementType"]["fields"]
    assert [field["name"] for field in fields] == ["count", "input0", "input1", "output0", "output1"], label
    assert [field["binding"]["offset"] for field in fields] == [0, 4, 8, 12, 16], label
    handles = []
    annotations = {}
    for line in result.stdout.splitlines():
        match = re.search(r"(%[A-Za-z0-9_.]+) = call %dx.types.Handle @dx.op.createHandleFromHeap\(i32 218, i32 (%[A-Za-z0-9_.]+), i1 false, i1 true\)", line)
        if match:
            handles.append({"handle": match[1], "index": match[2]})
        match = re.search(r"@dx.op.annotateHandle\(i32 216, %dx.types.Handle (%[A-Za-z0-9_.]+),.*resource: (RWStructuredBuffer|StructuredBuffer)<stride=(\d+)>", line)
        if match:
            annotations[f"{match[2]}:{match[3]}"] = match[1]
    assert "StructuredBuffer:16" in annotations and "RWStructuredBuffer:8" in annotations, (label, annotations)
    return {"binary_sha256": sha(binary), "reflection_sha256": sha(binary.with_suffix(".reflection.json")),
            "nonuniform_resource_handles": handles, "annotations": annotations,
            "disassembly_sha256": sha(args.output / f"{label}.dxil.txt")}

slang_version = subprocess.check_output([str(args.slang), "-version"], text=True, stderr=subprocess.STDOUT).strip()
assert slang_version == "2026.20", slang_version
stored = inspect("stored", args.stored)
direct = inspect("direct", args.direct)
assert len(direct["nonuniform_resource_handles"]) == 2, direct
assert len({value["index"] for value in direct["nonuniform_resource_handles"]}) == 2, direct
assert direct["annotations"]["StructuredBuffer:16"] != direct["annotations"]["RWStructuredBuffer:8"], direct
# Record the unsafe-expression behavior without requiring future compilers to retain the bug
hazard = (stored["annotations"]["StructuredBuffer:16"] == stored["annotations"]["RWStructuredBuffer:8"])
receipt = {"result": "PASS", "scope": "CPU DXIL compiler contract; neither diagnostic shader is dispatched",
           "slang_version": slang_version, "slang_sha256": sha(args.slang),
           "slang_runtime_sha256": {path.name: sha(path) for path in sorted(args.slang.parent.glob("slang*.dll"))},
           "source_sha256": sha(args.source), "cmake_driver_sha256": sha(args.driver),
           "target": "DXIL sm_6_6", "optimization": "O3",
           "dxc_sha256": sha(args.dxc), "dxcompiler_sha256": sha(args.dxc.with_name("dxcompiler.dll")),
           "stored_result_alias_observed": hazard, "stored": stored, "direct": direct}
(args.output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
print("Direct-subscript nonuniform SRV/UAV handles: PASS; stored-result alias observed:", hazard)
