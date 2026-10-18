# OpenRHI

- Preserve third-party copyrights, license terms, and the provenance recorded in THIRD_PARTY_NOTICES.md. Keep the GitHub upstream fork relationship and configured upstream remote. Published API, integration documentation, tags, and branches use OpenRHI identity.
- Preserve shared upstream ancestry on main so ahead/behind counts track actual pending updates. Record upstream commits as incorporated after reviewing and integrating their changes, including intentional platform exclusions; keep the recorded upstream base current in THIRD_PARTY_NOTICES.md. Refresh with `git fetch upstream`; `git rev-list --left-right --count upstream/main...main` reports upstream-only commits followed by OpenRHI-only commits.
- Public identity is OpenRHI: `OpenRHI.h`, namespace `openrhi`, C exports `openrhi*`, C types `OpenRhi*`, macros/options `OPENRHI_*`, target/library `OpenRHI`, SDK `_OpenRHI_SDK`.
- Spell the brand `OpenRHI` inside function names; extension descriptions use their declared `*Interface` names. Keep debugger visualizers bound to validation members rather than obsolete device-layout offsets. Source/export audits cover build helpers, shaders, tests, CI, debugger support, and packaging; third-party notices preserve upstream attribution.
- Windows 11 x64/ARM64 uses D3D12 with feature level 12_2, SM 6.6, and Enhanced Barriers. Pin stable Agility SDK 1.619.6.
- Linux x64/ARM64 and Android ARM64 API 33+ use Vulkan 1.3 minimum, 1.4 preferred. Require dynamic rendering, synchronization2, maintenance4, buffer addresses, timeline semaphores, runtime descriptor arrays, partially bound descriptors, and sampled-image nonuniform indexing.
- macOS/iOS/iPadOS 26+ uses native Metal 4 and Apple GPU family 9+. NONE is explicitly enabled for automated tests.
- Preserve retained graphics API numeric bit values and the C/C++ facade. Consumers rebuild against API revision 1.
- Explicit adapter selection succeeds on that device or returns its compatibility report with a null device. Automatic selection evaluates compatible devices and orders discrete GPUs, available memory budget, then UID.
- Capture device identity through native APIs. Separate supported/enabled capabilities, requested requirements, recommended policies, and measured qualification.
- Resolve profiles at creation. Keep frame execution allocation-free and synchronization/resource ownership explicit. Benchmark execution is an explicit tool invocation.
- Promote tuned policies only with five repeatable paired runs, correctness/native-validation success, no p95 regression above 5% elsewhere, and three 20-minute sustained runs for mobile. Keep per-device/OS/driver/backend receipts.
- Keep vendor dependencies unmodified and use their public interfaces.
- Apply `.agents/skills/openrhi-coding-standard/SKILL.md` and `.agents/skills/openrhi-review/SKILL.md`.
- Maintain selection, capability, source/export, C/C++, build, runtime, packaging, and promotion-gate tests. Report build coverage and measured runtime coverage separately.
- Move requested local removals to the operating system Recycle Bin/Trash, verify the exact target first, and verify the original path is gone afterward.
- macOS generated-directory cleanup uses Foundation's native Trash API, verifies the resolved repository target, and works in SSH sessions.

- Author every shader in Slang (.slang) and compile explicitly with pinned Slang 2026.20 through CMake/OpenRHISlang.cmake. Ship native DXIL, SPIR-V, and metallib outputs with reflection-derived entry and threadgroup metadata. Keep compiler work outside device creation and submission.
- Keep shader layouts explicit; native Metal resource tables use the documented 24-byte entry ABI. Use queried native FP16 capability when selecting half-precision variants.
- Keep reflected shader threadgroup dimensions and explicit task payload bytes paired with native binaries; Metal pipeline creation consumes both. Use conservative Metal NIS tile sizes that fit native GPU-validation threadgroup storage.
