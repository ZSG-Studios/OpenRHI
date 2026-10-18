import copy
import importlib.util
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("qualification", Path(__file__).resolve().parents[1] / "Scripts/Qualification/run.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def receipts():
    result = []
    for size in (65536, 4194304, 16777216):
        for run in range(5):
            for variant in ("staging", "device-upload"):
                sample = {"api": "D3D12", "vendor_id": 0x1002, "device_id": 0x7550, "revision_id": 192,
                          "driver": 1, "profile_revision": 1, "variant": variant, "bytes": size,
                          "validation_errors": 0, "cpu_ms": [1.0] * 32, "gpu_ms": [1.0 if variant == "staging" else .8] * 32}
                result.append({"sample": sample, "run": run, "source_dirty": False, "host": {}, "commit": "a", "binary_sha256": "b", "workload_sha256": "c", "power_state": "AC"})
    return result


class PromotionTests(unittest.TestCase):
    def test_native_diagnostics_do_not_hide_or_duplicate_samples(self):
        self.assertEqual(module.parse_sample('native diagnostic\n{"schema":1,"cpu_ms":[1]}\n')["cpu_ms"], [1])
        for output in ('', '{"schema":1,"cpu_ms":[1]}\n{"schema":1,"cpu_ms":[2]}'):
            with self.assertRaises(ValueError):
                module.parse_sample(output)

    def test_microbenchmark_alone_cannot_promote(self):
        result = module.compare(receipts())
        self.assertTrue(result["microbenchmark_gate_passed"])
        self.assertFalse(result["promotion_eligible"])

    def test_p95_regression_rejects(self):
        samples = receipts()
        samples[1]["sample"]["cpu_ms"][-3:] = [2.0] * 3
        self.assertFalse(module.compare(samples)["microbenchmark_gate_passed"])

    def test_driver_change_rejects(self):
        samples = receipts()
        samples[-1]["sample"]["driver"] = 2
        self.assertFalse(module.compare(samples)["microbenchmark_gate_passed"])

    def test_different_physical_device_rejects(self):
        samples = receipts()
        samples[-1]["sample"]["uid_low"] = 2
        self.assertFalse(module.compare(samples)["microbenchmark_gate_passed"])

    def test_missing_pair_rejects(self):
        self.assertFalse(module.compare(receipts()[:-1])["microbenchmark_gate_passed"])

    def test_validation_error_rejects(self):
        samples = receipts()
        samples[0]["sample"]["validation_errors"] = 1
        self.assertFalse(module.compare(samples)["microbenchmark_gate_passed"])

    def test_mobile_requires_sustained_execution(self):
        self.assertFalse(module.compare(receipts(), mobile=True)["microbenchmark_gate_passed"])

    def test_complete_matching_receipt_can_qualify_for_review(self):
        data = receipts()
        verification = {"commit": "a", "binary_sha256": "b", "host": {}, "native_validation_errors": 0,
                        "correctness": {name: True for name in ("upload_readback", "descriptor_indexing", "barriers", "compute", "indirect_draw", "queue_fence", "presentation")},
                        "cross_workloads": []}
        pair = {k: {metric: [1.0] * 32 for metric in ("cpu_ms", "gpu_ms")} for k in ("baseline", "candidate")}
        verification["cross_workloads"] = [{"name": name, "pairs": [copy.deepcopy(pair) for _ in range(5)]} for name in ("compute", "descriptors", "indirect_draw")]
        self.assertTrue(module.compare(data, verification=verification)["promotion_eligible"])
        verification["cross_workloads"][0]["pairs"][0]["candidate"]["gpu_ms"] = [1.1] * 32
        self.assertFalse(module.compare(data, verification=verification)["promotion_eligible"])


if __name__ == "__main__":
    unittest.main()
