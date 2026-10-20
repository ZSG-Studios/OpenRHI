"""Build/check on CPU by default; --native requires a separately coordinated GPU window."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
SDK = ROOT / "_OpenRHI_SDK"
SLANG = ROOT / "_Build/_deps/openrhi_slang-src"
DXC = ROOT / "_Build/_deps/openrhi_slang_dxil-src/bin/x64/dxc.exe"
SDK_HASHES = {
    "Release": "94c0326816c019792ed67df9dcf09db0b328498da62c215f43a03e16d75cd751",
    "Debug": "de945cb0317d63006947d477d6b110dda0a1eee0dfe79aff8ecae24f4d2ca34d",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(arguments, log):
    result = subprocess.run([str(x) for x in arguments], capture_output=True, text=True, errors="replace")
    log.write_text(result.stdout + result.stderr, encoding="utf-8")
    if result.returncode:
        raise RuntimeError(f"exit {result.returncode}: {arguments[0]}; {log}\n{result.stdout}{result.stderr}")
    return result.stdout


def compiler_contract(build, out):
    reports = {}
    for name in ["BindlessMaterialHeap", "BindlessMaterialConventional"]:
        binary = build / (name + ".dxil")
        reflection = json.loads(binary.with_suffix(".reflection.json").read_text())
        entry = next(x for x in reflection["entryPoints"] if x["name"] == "materialMain")
        assert entry["stage"] == "compute" and entry["threadGroupSize"] == [64, 1, 1]
        root = next(x for x in reflection["parameters"] if x["name"] == "root")
        assert root["binding"] == {"kind": "constantBuffer", "space": 1, "index": 0}
        fields = root["type"]["elementType"]["fields"]
        assert [x["name"] for x in fields] == ["casesIndex", "outputIndex", "firstCase", "caseCount"]
        assert [x["binding"]["offset"] for x in fields] == [0, 4, 8, 12]
        disassembly = run([DXC, "-dumpbin", binary], out / (name + ".dxil.txt"))
        assert "StructuredBuffer<stride=48>" in disassembly and "RWStructuredBuffer<stride=32>" in disassembly
        assert "@dx.op.sampleGrad.f32" in disassembly and "@dx.op.sampleLevel.f32" in disassembly
        if name.endswith("Heap"):
            assert len(reflection["parameters"]) == 1, "heap shader must have only root binding"
            handles = re.findall(r"@dx.op.createHandleFromHeap\(i32 218, i32 (%[\w.]+), i1 (false|true), i1 true\)", disassembly)
            assert len(handles) == 2 and {x[1] for x in handles} == {"false", "true"}, handles
            assert len({x[0] for x in handles}) == 2, handles
        else:
            assert "@dx.op.createHandleFromHeap" not in disassembly
            assert {x["name"] for x in reflection["parameters"]} == {"root", "cases", "results", "source", "sampling"}
            record_fields = {
                "cases": {"textureIndex": 0, "samplerIndex": 4, "operation": 8, "reserved": 12, "uv": 16, "lod": 24, "padding": 28, "dx": 32, "dy": 40},
                "results": {"color": 0, "width": 16, "height": 20, "levels": 24, "reserved": 28},
            }
            for parameter, expected in record_fields.items():
                record = next(x for x in reflection["parameters"] if x["name"] == parameter)["type"]["resultType"]
                assert {x["name"]: x["binding"]["offset"] for x in record["fields"]} == expected
        reports[name] = {"binary_sha256": sha(binary), "reflection_sha256": sha(binary.with_suffix(".reflection.json")),
                         "disassembly_sha256": sha(out / (name + ".dxil.txt"))}
    return reports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "_Build/BindlessMaterial")
    parser.add_argument("--out", type=Path, default=ROOT / ("_Tmp/BindlessMaterial/" + datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")))
    parser.add_argument("--native", action="store_true", help="execute native fixture during an allocated GPU window")
    parser.add_argument("--no-build", action="store_true", help="reuse already verified build during the GPU window")
    parser.add_argument("--anisotropy", type=int, choices=[1, 16], default=1, help="separately labeled native sampler profile")
    args = parser.parse_args()
    build, out = args.build.resolve(), args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    receipt = {"status": "started", "gpu_requested": args.native, "gpu_executed": False,
               "sdk_dll_sha256": SDK_HASHES, "configurations": {},
               "profile": {"reference_policy": 2, "formats": ["BC4_R_UNORM", "BC5_RG_UNORM", "BC7_RGBA_UNORM", "BC7_RGBA_SRGB"],
                           "dimensions": [[20, 12], [17, 9], [4, 20]], "samplers": ["point-clamp", "point-repeat", "linear-clamp", "linear-repeat"],
                           "anisotropy": 1, "mip_min": 0, "mip_max": 32, "mip_bias": 0,
                           "operations": ["fixed-view-mip-zero", "explicit-view-relative-LOD", "isotropic-explicit-gradients"],
                           "point_absolute_tolerance": 1 / 255 + 1e-5, "linear_absolute_tolerance": 2 / 255 + 1e-5,
                           "heap_conventional_absolute_tolerance": 1e-6,
                           "bounded_value_arithmetic_allowance": "same predeclared point/linear filter allowance; never a patterned anisotropic quality reference",
                           "bc4_bc5_decode_interval": "per-texel 1/65535 + 0.03*abs(endpoint0-endpoint1); exact endpoints/special values; propagated through the actual reference filter",
                           "bc7_blocks": "mode 6 only; all-mode compression decode remains separately gated",
                           "odd_originals": "successful exact public factory creation required; SDK94 has no public unaligned-BC capability query"}}
    if args.anisotropy == 16:
        receipt["profile"].update(anisotropy=16, dimensions=[[20, 12], [4, 20]],
                                  samplers=["linear-clamp-anisotropy16", "linear-repeat-anisotropy16"],
                                  operations=["fixed-view-mip-zero", "explicit-view-relative-LOD", "isotropic-gradients", "anisotropic-gradients-4x1-8x1-16x1"],
                                  reference_scope="scalar ideal/interval checks for fixed/LOD/isotropic and constant-field gradients; patterned anisotropic gradients use independent admitted-view value bounds, finite checks and native parity only")
    sources = [HERE / x for x in ["Main.cpp", "Reference.h", "Material.slang", "CMakeLists.txt", "qualify.py", "README.md"]]
    sources.append(ROOT / "Tests/CMakeLists.txt")
    sources.append(ROOT / "Tests/SourceBoundary.cmake")
    receipt["source_sha256"] = {str(p.relative_to(ROOT)): sha(p) for p in sources}
    receipt["driver_sha256"] = sha(SDK / "CMake/OpenRHISlang.cmake")
    receipt["compiler_sha256"] = {str(p): sha(p) for p in [SLANG / "bin/slangc.exe", DXC, DXC.with_name("dxcompiler.dll")]}
    receipt["slang_runtime_sha256"] = {p.name: sha(p) for p in sorted((SLANG / "bin").glob("slang*.dll"))}
    try:
        native_failures = []
        run([SLANG / "bin/slangc.exe", "-version"], out / "slang-version.log")
        receipt["slang_version"] = (out / "slang-version.log").read_text().strip()
        assert receipt["slang_version"] == "2026.20"
        baseline = None
        if args.no_build:
            baseline = json.loads((build / "cpu-build-receipt.json").read_text())
            assert baseline["status"] == "cpu_build_passed_native_pending"
            assert baseline["source_sha256"] == receipt["source_sha256"], "source changed since CPU build"
            assert baseline["compiler_sha256"] == receipt["compiler_sha256"] and baseline["driver_sha256"] == receipt["driver_sha256"]
        for config, expected in SDK_HASHES.items():
            assert sha(SDK / f"Lib/{config}/OpenRHI.dll") == expected, "qualified SDK changed"
        if not args.no_build:
            run(["cmake", "-S", HERE, "-B", build, "-G", "Visual Studio 18 2026", "-A", "x64",
                 f"-DOPENRHI_SLANG_ROOT={SLANG}", f"-DOPENRHI_SLANG_DXIL_ROOT={DXC.parents[2]}"], out / "configure.log")
        for config, expected in SDK_HASHES.items():
            if not args.no_build:
                run(["cmake", "--build", build, "--config", config, "--parallel", "4"], out / f"{config}-build.log")
            exe = build / config / "OpenRHIBindlessMaterial.exe"
            local_dll = exe.with_name("OpenRHI.dll")
            assert sha(local_dll) == expected, "isolated SDK DLL mismatch"
            receipt["shader_contract"] = compiler_contract(build, out)
            if baseline:
                assert baseline["shader_contract"] == receipt["shader_contract"], "shader changed since CPU build"
                assert baseline["configurations"][config]["executable_sha256"] == sha(exe), "executable changed since CPU build"
            cpu = json.loads(run([exe, "--cpu-only"], out / f"{config}-cpu.json"))
            assert cpu["status"] == "cpu_reference_passed" and not cpu["gpu_executed"]
            entry = {"cpu": cpu, "executable_sha256": sha(exe), "loaded_library_candidate": str(local_dll), "library_sha256": sha(local_dll)}
            receipt["configurations"][config] = entry
            if args.native:
                native = out / (config + "-native")
                frozen = out / (config + "-frozen")
                frozen.mkdir()
                for artifact in [exe, local_dll, build / "BindlessMaterialHeap.dxil", build / "BindlessMaterialConventional.dxil",
                                 build / "BindlessMaterialHeap.reflection.json", build / "BindlessMaterialConventional.reflection.json"]:
                    shutil.copy2(artifact, frozen / artifact.name)
                for source in sources:
                    destination = frozen / "Source" / source.relative_to(ROOT)
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(source, destination)
                entry["frozen_sha256"] = {str(p.relative_to(out)): sha(p) for p in sorted(frozen.rglob("*")) if p.is_file()}
                try:
                    # Set immediately before invocation, after the snapshot is complete
                    receipt["gpu_executed"] = True
                    stdout = run([exe, "--native-aniso16" if args.anisotropy == 16 else "--native", build / "BindlessMaterialHeap.dxil", build / "BindlessMaterialConventional.dxil", native], out / f"{config}-native.log")
                    summary = json.loads(next(line for line in stdout.splitlines() if line.startswith('{"status":"native_passed"')))
                    assert summary["validation_errors"] == 0 and summary["profiles"] == (32 if args.anisotropy == 16 else 24) and summary["cases"] == (36864 if args.anisotropy == 16 else 41472)
                    assert summary["anisotropy"] == args.anisotropy
                    entry["native_summary"] = summary
                except Exception as error:
                    native_failures.append(config)
                    entry["native_failure"] = str(error)
                finally:
                    if (native / "device.json").exists():
                        identity = json.loads((native / "device.json").read_text())
                        assert Path(identity["library_path"]).resolve() == local_dll.resolve()
                        assert sha(Path(identity["library_path"])) == expected
                        if args.anisotropy == 16:
                            factories = {x["slot"]: x for x in identity["sampler_factories"]}
                            for slot, address in [(31, 2), (43, 0)]:
                                sampler = factories[slot]
                                assert sampler["success"] and sampler["filters"] == [1, 1, 1] and sampler["filter_op"] == 0
                                assert sampler["address"] == [address] * 3 and sampler["anisotropy"] == 16
                                assert sampler["mip_bias"] == 0 and sampler["mip_min"] == 0 and sampler["mip_max"] == 32
                                assert sampler["compare"] == 0 and not sampler["integer"] and not sampler["unnormalized"]
                                assert sampler["border_u32"] == [0, 0, 0, 0]
                        entry["device"] = identity
                    entry["artifact_sha256"] = {str(p.relative_to(out)): sha(p) for p in sorted(native.rglob("*")) if p.is_file()}
        for config, expected in SDK_HASHES.items():
            assert sha(SDK / f"Lib/{config}/OpenRHI.dll") == expected, "qualified SDK changed during test"
        if native_failures:
            raise RuntimeError("native configuration failures retained: " + ", ".join(native_failures))
        receipt["status"] = "native_passed" if args.native else "cpu_build_passed_native_pending"
    except Exception as error:
        receipt["status"] = "failed"
        receipt["error"] = str(error)
        raise
    finally:
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    if not args.native and not args.no_build:
        (build / "cpu-build-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": receipt["status"], "gpu_executed": receipt["gpu_executed"], "receipt": str(out / "receipt.json")}))


if __name__ == "__main__":
    main()
