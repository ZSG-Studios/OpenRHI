# RX 9070 XT on Windows / D3D12

Device identity: PCI `1002:7550`, revision `C0`, native LUID `68064:0`. Driver `32.0.32015.2008`. Windows 11 Pro for Workstations, build 26300. Toolchain: MSVC 19.51.36260.0, Windows SDK 10.0.28000.0, stable Agility SDK 1.619.6, x64 Release.

## Compatibility

The RX 9070 XT meets FL 12_2, SM 6.6, and Enhanced Barrier requirements; it reports SM 6.9. Explicit selection preserves the requested UID. Selection tests also cover the integrated AMD adapter, the software adapter, impossible memory budgets, disabled Enhanced Barriers, stale UIDs, and deterministic automatic preference. See `compatibility.json` for the exact native reports.

## Correctness coverage

`correctness.json` records separate staging and GPU-upload runs with native validation. Both verify uploads/readback, explicit barriers, descriptor-array compute output, indirect drawing, queue/fence completion, and hidden-window swapchain presentation. The fixture uses a graphics queue. Cross-queue overlap/ownership and optional mesh, ray-tracing, video, and upscaler paths remain additional qualification work. The native profile therefore remains unqualified.

## Performance

`measurements/` contains five paired runs at each of 64 KiB, 4 MiB, and 16 MiB: 30 receipts with 128 measured iterations after eight warmups, exact executable/library/workload/shader hashes, raw CPU/GPU samples, native memory use, and host/power metadata. The CPU timing includes upload, recording, submission, and completion; GPU timing covers copies. Readback is checked outside the timed interval. This is a desktop-session microbenchmark; it measures payload throughput and does not establish rendering-workload performance.

`measurements/summary.json` is the promotion-gate result. A tuned production policy requires repeatable gains, tail-latency gates, and complete matching cross-workload correctness/performance evidence. Tuning stays disabled.

Historical data is retained: `initial-4ee588b/` includes a toolchain-string correction note; `incomplete-90bbc4c/` is explicitly excluded from qualification; `measured-15cb192/` records the completed earlier source revision. Each receipt identifies its own measured commit and binary. Later documentation commits do not alter those identities.

## Reproduction

Build the recorded commit with tests and NONE explicitly enabled, then run the commands in `QUALIFICATION.md` using `--uid-low 68064`. Native UIDs can change after a reboot; enumerate and select the matching physical PCI identity before repeating on another session. Record the current power plan, driver, compiler, and SDK versions. Compare only receipts from the same device, driver, workload, binaries, and power configuration.

## Measured upload timings

Median of five run medians; milliseconds. These copy microbenchmarks describe the recorded commit, not a promoted rendering policy.

| Payload | Staging CPU | Candidate CPU | Staging GPU | Candidate GPU |
|---|---:|---:|---:|---:|
| 65536 bytes | 0.090450 | 0.062350 | 0.004600 | 0.002120 |
| 4194304 bytes | 0.400900 | 0.322600 | 0.101280 | 0.021200 |
| 16777216 bytes | 1.284650 | 0.972050 | 0.553340 | 0.242740 |

Promotion result: **not qualified**. Benefit/tail gate failed: bytes=65536, run=2; Complete correctness and cross-workload verification receipt required.
Performance receipts identify commit `c307e4d`. Compatibility/correctness and SDK checks identify their own later source commit after the failure-reporting fix.
