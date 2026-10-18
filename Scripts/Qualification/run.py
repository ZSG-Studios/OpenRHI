#!/usr/bin/env python3
"""Explicit, reproducible upload-policy comparisons; never runs during device creation."""
import argparse
import hashlib
import json
import math
import platform
import statistics
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def command(args, cwd=ROOT):
    return subprocess.check_output(args, cwd=cwd, text=True, stderr=subprocess.STDOUT).strip()


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def percentile(values, fraction):
    return sorted(values)[max(0, math.ceil(len(values) * fraction) - 1)]


def summarize(sample):
    return {name: {"median": statistics.median(sample[name]), "p95": percentile(sample[name], .95)}
            for name in ("cpu_ms", "gpu_ms")}


def parse_sample(stdout):
    samples = []
    for line in stdout.splitlines():
        try:
            value = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(value, dict) and value.get("schema") == 1 and "cpu_ms" in value:
            samples.append(value)
    if len(samples) != 1:
        raise ValueError("Expected exactly one qualification sample; inspect the saved stdout/stderr")
    return samples[0]


def compare(receipts, mobile=False, verification=None):
    """Every workload must repeat a benefit in five pairs and obey both tail-latency gates."""
    groups = {}
    for receipt in receipts:
        sample = receipt["sample"]
        key = (sample["bytes"], receipt["run"])
        groups.setdefault(key, {})[sample["variant"]] = receipt
    failures, comparisons = [], []
    identities = {(r["sample"]["api"], r["sample"]["vendor_id"], r["sample"]["device_id"],
                   r["sample"]["revision_id"], r["sample"]["driver"], r["sample"]["profile_revision"],
                   r["sample"].get("uid_low"), r["sample"].get("uid_high"), r.get("toolchain"), r.get("shader_sha256"),
                   json.dumps(r["host"], sort_keys=True), r["commit"], r["binary_sha256"],
                   r["workload_sha256"], r.get("library_sha256"), r["power_state"]) for r in receipts}
    if len(identities) != 1:
        failures.append("Device, driver, host, binary, workload, commit, or power configuration differs")
    if any(r.get("source_dirty", True) for r in receipts):
        failures.append("Source tree must be clean for every measurement")
    workload_runs = {}
    for (size, run), pair in sorted(groups.items()):
        if set(pair) != {"staging", "device-upload"}:
            failures.append(f"Missing comparison: bytes={size}, run={run}")
            continue
        baseline, candidate = pair["staging"], pair["device-upload"]
        if any(r["sample"]["validation_errors"] != 0 for r in pair.values()):
            failures.append(f"Validation error: bytes={size}, run={run}")
        valid = all(len(r["sample"].get(metric, [])) >= 32 and all(v > 0 and math.isfinite(v) for v in r["sample"][metric]) for r in pair.values() for metric in ("cpu_ms", "gpu_ms"))
        if not valid:
            failures.append(f"Invalid or insufficient timing samples: bytes={size}, run={run}")
            continue
        b, c = summarize(baseline["sample"]), summarize(candidate["sample"])
        cpu = c["cpu_ms"]["median"] / b["cpu_ms"]["median"]
        gpu = c["gpu_ms"]["median"] / b["gpu_ms"]["median"]
        tail_ok = all(c[k]["p95"] <= b[k]["p95"] * 1.05 for k in b)
        benefit = min(cpu, gpu) < .98
        if not tail_ok or not benefit:
            failures.append(f"Benefit/tail gate failed: bytes={size}, run={run}")
        workload_runs.setdefault(size, set()).add(run)
        comparisons.append({"bytes": size, "run": run, "cpu_median_ratio": cpu,
                            "gpu_median_ratio": gpu, "tail_gate_passed": tail_ok,
                            "payload_throughput_gib_s": {variant: {metric: size * 1000 / (values[metric]["median"] * 1024**3) for metric in ("cpu_ms", "gpu_ms")} for variant, values in (("baseline", b), ("candidate", c))},
                            "baseline": b, "candidate": c})
    if len(workload_runs) < 3 or any(len(runs) < 5 for runs in workload_runs.values()):
        failures.append("Requires three workload sizes and five paired runs per size")
    if mobile:
        sustained = [r for r in receipts if r["sample"].get("sustained_seconds", 0) >= 1200]
        if any(len({r["run"] for r in sustained if r["sample"]["variant"] == variant}) < 3 for variant in ("staging", "device-upload")):
            failures.append("Requires three sustained mobile sessions of at least 1200 seconds")
    promotion_failures = list(failures)
    required_cases = {"upload_readback", "descriptor_indexing", "barriers", "compute", "indirect_draw", "queue_fence", "presentation"}
    if not verification:
        promotion_failures.append("Complete correctness and cross-workload verification receipt required")
    else:
        first = receipts[0] if receipts else {}
        if verification.get("commit") != first.get("commit") or verification.get("binary_sha256") != first.get("binary_sha256") or verification.get("host") != first.get("host") or verification.get("library_sha256") != first.get("library_sha256"):
            promotion_failures.append("Verification receipt does not match measured source, binary, and host")
        checked = verification.get("correctness", {})
        if not required_cases <= checked.keys() or not all(checked.get(k) is True for k in required_cases):
            promotion_failures.append("Required correctness scenarios are incomplete")
        if verification.get("native_validation_errors") != 0:
            promotion_failures.append("Native validation must report zero errors")
        other = verification.get("cross_workloads", [])
        if not {"compute", "descriptors", "indirect_draw"} <= {w.get("name") for w in other}:
            promotion_failures.append("Compute, descriptor, and indirect-draw comparison workloads required")
        for workload in other:
            pairs = workload.get("pairs", [])
            if len(pairs) < 5:
                promotion_failures.append("Cross-workload comparison requires five pairs")
            for pair in pairs:
                for metric in ("cpu_ms", "gpu_ms"):
                    baseline, candidate = pair.get("baseline", {}).get(metric, []), pair.get("candidate", {}).get(metric, [])
                    if not baseline or not candidate or any(v <= 0 or not math.isfinite(v) for v in baseline + candidate):
                        promotion_failures.append("Invalid cross-workload timing samples")
                    elif percentile(candidate, .95) > percentile(baseline, .95) * 1.05:
                        promotion_failures.append("Cross-workload p95 regression exceeds 5%")
    return {"microbenchmark_gate_passed": not failures, "promotion_eligible": not promotion_failures,
            "promotion_status": "eligible_for_review" if not promotion_failures else "not_qualified",
            "failures": failures, "promotion_failures": promotion_failures, "comparisons": comparisons}


def host_metadata():
    metadata = {"system": platform.system(), "release": platform.release(),
                "version": platform.version(), "machine": platform.machine()}
    if platform.system() == "Windows":
        script = "@{os=Get-CimInstance Win32_OperatingSystem|Select-Object Caption,BuildNumber;bios=Get-CimInstance Win32_BIOS|Select-Object SMBIOSBIOSVersion;gpu=Get-CimInstance Win32_VideoController|Select-Object Name,PNPDeviceID,DriverVersion} | ConvertTo-Json -Depth 5 -Compress"
        metadata["native"] = json.loads(command(["powershell", "-NoProfile", "-Command", script]))
        metadata["power_plan"] = command(["powercfg", "/getactivescheme"])
    return metadata


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--api", choices=["D3D12", "VK", "METAL"], required=True)
    parser.add_argument("--shader", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--power-state", required=True)
    parser.add_argument("--toolchain", required=True)
    parser.add_argument("--iterations", type=int, default=128)
    parser.add_argument("--mobile", action="store_true")
    parser.add_argument("--uid-low", help="Explicit native adapter UID, decimal or 0x hexadecimal")
    parser.add_argument("--uid-high", default="0")
    parser.add_argument("--verification", type=Path, help="Matching correctness and cross-workload receipt")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    metadata = host_metadata()
    runner = args.runner.resolve()
    library = runner.parent / ("OpenRHI.dll" if platform.system() == "Windows" else ("libOpenRHI.dylib" if platform.system() == "Darwin" else "libOpenRHI.so"))
    base = {"schema": 1, "commit": command(["git", "rev-parse", "HEAD"]),
            "source_dirty": bool(command(["git", "status", "--porcelain", "--untracked-files=no"])),
            "binary_sha256": digest(runner), "library_sha256": digest(library) if library.exists() else None, "workload_sha256": digest(ROOT / "Tests/DeviceQualification.cpp"),
            "shader_sha256": digest(args.shader) if args.shader else None,
            "toolchain": args.toolchain, "build_configuration": "Release",
            "host": metadata, "power_state": args.power_state}
    receipts = []
    for run in range(5):
        for size in (65536, 4194304, 16777216):
            variants = ("staging", "device-upload") if run % 2 == 0 else ("device-upload", "staging")
            for variant in variants:
                cmd = [str(runner), "--api", args.api, "--variant", variant,
                       "--bytes", str(size), "--iterations", str(args.iterations)]
                if args.shader:
                    cmd += ["--shader", str(args.shader.resolve())]
                if args.uid_low is not None:
                    cmd += ["--uid-low", args.uid_low, "--uid-high", args.uid_high]
                if args.mobile and run < 3 and size == 4194304:
                    cmd += ["--seconds", "1200"]
                started = time.time()
                result = subprocess.run(cmd, capture_output=True, text=True)
                stem = f"run-{run}-{size}-{variant}"
                (args.output / (stem + ".stdout.txt")).write_text(result.stdout, encoding="utf-8")
                (args.output / (stem + ".stderr.txt")).write_text(result.stderr, encoding="utf-8")
                if result.returncode == 77:
                    (args.output / (stem + ".skip.json")).write_text(json.dumps({"reason": result.stderr, "run": run, "variant": variant, "bytes": size}), encoding="utf-8")
                    continue
                if result.returncode:
                    raise RuntimeError(result.stderr)
                sample = parse_sample(result.stdout)
                if sample.get("variant") != variant or sample.get("bytes") != size:
                    raise ValueError("Qualification sample does not match the requested workload")
                receipt = {**base, "run": run, "elapsed_seconds": time.time() - started, "sample": sample}
                (args.output / (stem + ".json")).write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
                receipts.append(receipt)
                print(f"{stem}: checked", flush=True)
    verification = json.loads(args.verification.read_text()) if args.verification else None
    summary = {"schema": 1, "receipt_count": len(receipts), "source_dirty": base["source_dirty"], **compare(receipts, args.mobile, verification)}
    if base["source_dirty"]:
        summary["failures"].append("Source tree was dirty during measurement")
        summary["microbenchmark_gate_passed"] = False
        summary["promotion_eligible"] = False
        summary["promotion_status"] = "not_qualified"
    (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in summary.items() if k != "comparisons"}, indent=2))


if __name__ == "__main__":
    main()
