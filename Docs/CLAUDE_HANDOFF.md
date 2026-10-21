# Claude handoff: OpenRHI as a MIRA module

Snapshot: 20 October 2026, America/New_York. Scope is MIRA and its modules. This handoff preserves OpenRHI-owned context; MIRA MAIN owns the consolidated handoff and application sequencing. Preparation changes this document only: no commit, push, deletion, SDK replacement or native run.

## Copy/paste starter

> Work on OpenRHI's role in MIRA. Start in `C:/MIRA/Modules/LocalSources/OpenRHI`; read `AGENTS.md`, both `.agents/skills/openrhi-*/SKILL.md`, `Docs/mira-integration.md`, `Docs/QUALIFICATION.md`, and this handoff. Read MIRA's `modules.json`, `Docs/LOCAL_SOURCES.md`, `Docs/LIGHTING_PRIORITY.md` and current `Docs/plan.md` before selecting work. Working UVRT lighting and smooth first-lit startup are the priority. Preserve SDK94 until a concrete backend/public-contract blocker requires a separately qualified candidate. All owned builds use Xmake3.1.2 and Slang2026.20. Confirm the selected checkout, SDK, source/provider/shader identities and current dirty state; integrated and development roots differ. Coordinate native GPU work through MAIN's explicit serialized lease, reporting child PID and terminal/reaped release. Preserve failures and predeclared bounds. Keep ownership, actual output quality, first-lit/presentation latency and full dynamic coverage distinct from source/build/component evidence. FSR warm-up implementation and additional LargeX probes are on hold. Begin with MAIN's current lighting-critical request; do not infer production completion from old receipts.

## Purpose and current objective

OpenRHI owns the public C/C++ RHI, native device/backends, capabilities, shader toolchain, vendor-provider adapters, SDK and native component tests. It supplies MIRA's device/resource/AS/descriptor/queue/presentation boundary. UVRT owns illumination and lighting algorithms; FACET owns geometry, materials exported as attributes, streaming and temporal guides. MIRA owns application lifecycle, shared registries/adapters, coherent publication, scheduling, loading/UI/display composition and integrated qualification. BlenderAssets feeds authored revisions through FACET/MIRA; it does not own native RHI execution.

The latest substantive request to this chat was bounded read-only FSR first-use advice. It found no demonstrated SDK blocker. MAIN explicitly held implementation/diagnostics because measured renderer initialization and combined AS/metadata publication dominated startup. Subsequent source relocation and Xmake migration are current adopted workspace decisions. This handoff request authorizes preservation only.

## Checkouts, branches, provider selection and drift

| Item | Verified snapshot |
| --- | --- |
| Development owner checkout; handoff location | `C:/MIRA/Modules/LocalSources/OpenRHI` |
| Development branch / HEAD | `main` / `b737929aa5732f8f5e045d314b46438902d8eff8` |
| Pinned integrated checkout | `C:/MIRA/Modules/OpenRHI` |
| Integrated branch / HEAD | `mira/source-consolidation-20261020` / `186002f940e97861c20e71d510543a1f1bfabc1c` |
| Parent MIRA HEAD when inspected | `72015bfa56cf65298cd6701322ef86dea6764809` |
| Dirty state | Both OpenRHI checkouts and parent MIRA were clean at initial inspection. Final module validation shows only this untracked `Docs/CLAUDE_HANDOFF.md` in development; integrated OpenRHI remains clean. Concurrent parent handoff preparation now shows parent `Docs/CLAUDE_HANDOFF.md` and untracked content in `Modules/FACET`; this chat did not edit those files. Recheck before further work. |
| Repository association | Both OpenRHI checkouts use `https://github.com/ZSG-Studios/OpenRHI.git`. Development also retains configured `upstream`; preserve its remote, ancestry and notices. No fetch or remote synchronization was performed for this handoff. |
| Selected development source | Ignored `C:/MIRA/.mira-local.json` selects development OpenRHI, integrated FACET and integrated UVRT. `C:/MIRA/modules.json` records canonical integrated roots and owner chats. `C:/MIRA/local-sources.json` records development revisions. Environment overrides may change selection. |

These are independent Git checkouts, not aliases. Use real directories and explicit configured roots; preserve the no-symlink/no-junction decision. `C:/OpenRHI` is absent after relocation. Existing module instructions deliberately identify the development root even when copied into the integrated checkout; resolve the role through `Docs/LOCAL_SOURCES.md` rather than assuming the current directory is canonical development.

A focused byte comparison found the two checkouts identical for `Source/D3D12/CommandBufferD3D12.hpp`, `Source/Shared/UpscalerInterface.hpp`, `Include/Extensions/OpenRHIUpscaler.h`, `Docs/QUALIFICATION.md` and `xmake.lua`. `Docs/BC_TEXTURE_DIMENSIONS.md` differs: development contains the later confirmed full-mip receipt. This is a sampled comparison, not whole-tree equivalence or an ahead/behind conclusion. Update module pins only through MIRA's owning integration process.

The preserved development `_OpenRHI_SDK/Lib/Release/OpenRHI.dll` hashes to SDK94 `94c0326816c019792ed67df9dcf09db0b328498da62c215f43a03e16d75cd751`; Debug hashes to `de945cb0317d63006947d477d6b110dda0a1eee0dfe79aff8ecae24f4d2ca34d`. Both were rehashed during handoff. A selected source root, newly built native library and deployed qualified SDK remain separate identities. New Xmake outputs are not automatically replacements for SDK94.

## Binding instructions and decisions

- Working UVRT lighting takes priority; choose concrete backend/API/toolchain prerequisites for that path. Preserve vendor inputs, copyrights, licenses and provenance. Use public adapters. API revision1 preserves the C/C++ facade; consumers rebuild against changed headers.
- The first displayed scene must have valid lighting. Explicit loading UI is the startup path while it prepares; unlit inspection is a deliberate user choice. Retain process-to-first-lit time and actual loading/activation/presented-frame p95/p99/max intervals. HWND response alone is insufficient.
- MIRA defaults: FSR Native AA and Anti-Lag2 on; Frame Generation and VSync off. Keep independent user controls. `QueuePresent`'s final argument is a presentation ID; zero means untracked. Ordinary VSync belongs to swapchain creation flags.
- Every production profile shares one numeric combined operating envelope: resolution/quality, player/object/light/deformation counts, motion rates, streaming/churn, response/image/service ages, CPU/GPU frame deadlines, queue/memory limits, boundary pressure and recovery. Required unfinished systems stay in the denominator. UVRT's documented initial ledger is 154 OPEN / zero PASS.
- Admit immutable pipeline/cache inputs; retain device/layout/cache/bytecode through joined work. Caller collects/publicizes complete results. Publish matching current AS/geometry/material/resource generations after successful completion; retain recording/publication leases and last-use fences before reuse or retirement.
- Shared shader transport applies `NonUniformResourceIndex` directly at each nonuniform heap subscript. Preserve the pinned compiler regression, typed factory provenance, generation checks and native readback gates. Descriptor kind alone does not prove raw/structured/view compatibility.
- D3D12 AMD handoffs use COMMON/GENERAL/ALL; retain borrowed input owners and restore consuming states/bindings. Keep HUD-free scene color separate from UI. Deconfigure/retire FG before replacing borrowed SR outputs.
- Windows uses D3D12 feature level12_2, SM6.6, Enhanced Barriers and Agility1.619.6; Linux/Android use the admitted Vulkan contract; Apple uses native Metal4. Cross-compilation/reflection and actual device qualification are separate.
- Xmake3.1.2 is the sole owned build graph, including shaders, third-party public source targets, fixtures, tests, CI and SDK packaging. Slang2026.20 uses `Xmake/openrhi_slang.lua`. Module native code is C++17; MIRA application code is C++23. Preserve CRLF/UTF-8 and current source organization/Validation rules.
- Native work requires MAIN's explicit serialized window. A released lease does not authorize another run. Preserve exact failures, limits and denominators. Promotion requires five paired runs, correctness/validation, no p95 regression over5% elsewhere and mobile sustained gates where applicable.
- Requested removal goes to the OS Recycle Bin/Trash after exact path verification. This handoff performs no cleanup. Speak in positive supported capabilities and report measured scope candidly.

## Authoritative files and interfaces

All module-relative paths below resolve under the development root above:

| Area | Owning files |
| --- | --- |
| Instructions/review | `AGENTS.md`; `.agents/skills/openrhi-coding-standard/SKILL.md`; `.agents/skills/openrhi-review/SKILL.md` |
| Integration and qualification | `Docs/mira-integration.md`; `Docs/QUALIFICATION.md`; `Docs/BINDLESS.md`; `Docs/BC_TEXTURE_DIMENSIONS.md` |
| Build/provenance | `Docs/XMAKE.md`; `xmake.lua`; `Xmake/tests.lua`; `Xmake/dependencies.json`; `Scripts/Build/`; `THIRD_PARTY_NOTICES.md`; `Docs/UPSTREAM.md` |
| Public native facade | `Include/OpenRHI.h`; `Include/OpenRHIDescs.h`; `Include/Extensions/OpenRHIDeviceCreation.h`; `OpenRHIRayTracing.h`; `OpenRHIDescriptorHeap.h`; `OpenRHIUpscaler.h`; `OpenRHIFrameGeneration.h`; `OpenRHIVendorFeatures.h` in that extensions directory |
| Options/UI | `Include/Extensions/OpenRHIOptions.hpp`; `OpenRHIRmlUi.h`; `OpenRHIRmlUiAdapter.hpp`; `Docs/OPTIONS.md`; `Docs/UI.md`; `Docs/FRAME_GENERATION.md` |
| Backend paths discussed here | `Source/D3D12/CommandBufferD3D12.hpp`; `Source/D3D12/TextureD3D12.h`; `Source/Shared/SharedExternal.h`; `Source/Shared/UpscalerInterface.hpp`; `Source/Validation/` |

MIRA owns `C:/MIRA/Source/Runtime/TriangleScene.cpp`, `Source/Runtime/FsrPass.cpp`, `Include/MIRA/FsrPass.h`, `Source/UI/VendorOptions.cpp`, `Source/Demo/Main.cpp` and the current completed-work/loading protocol. Consult MAIN before changing those files. Current composition authority is `C:/MIRA/Docs/plan.md` and `Docs/LIGHTING_PRIORITY.md`; root selection is `Docs/LOCAL_SOURCES.md`. UVRT's development `plan.md` and `docs/dynamic-coverage.md` remain algorithm/coverage references; `.mira-local.json` currently consumes integrated UVRT, so compare its revision before assuming development work is active.

## Implemented, measured and open status

| Topic | Evidence and limits |
| --- | --- |
| Xmake migration | Implemented in development `b737929a` and integrated `186002f9`. `Logs/XmakeMigration/receipt.json` records Windows x64 full-feature Release compilation, NONE/host checks, 7/7 Release CPU checks, corrected D3D12 smoke and adapter checks, eight-process cold acquisition, 2/2 optional-runtime packaging and 9/9 qualification logic tests. Handoff rechecked all six `source_hashes` entries successfully. The receipt also contains older `helpers` hashes: do not assume every hash section names the final tree. Cross-platform and advanced provider runtime gates remain separate. |
| SDK-only V5 PSO control | `_Tmp/ExactPsoControlV5/Runs/Native-20261020-Initial/receipt.json`: PID24212 reaped exit0; exact V5 public pipeline creation 88632.3114ms; process93.661s before cleanup; zero errors and one optional-AGS warning. It establishes creation in that configuration, not current startup speed or the cause of an earlier application exit. |
| Current application startup | Last report to this chat: primary bootstrap loading about498ms / first lit about8.2s; warm pipeline hit about1ms with zero new creation. Renderer initialization about3.53s and combined publication about1.1s remained dominant. These are historical MAIN reports, not handoff reruns or verified latest application performance. Original startup deadline failures remain; consult MAIN's current receipts/candidate. |
| LargeX contact | `_Tmp/LargeXVisibilityControlV2/Runs/Native-20261020-Initial/review-receipt.json`: PID17544 reaped exit0 in2.561s; transport/guards/repeats/retirement and validation0 passed, but overall original-world visibility FAILED. Original world-coordinate BLAS misses both policies; recentered and translated-centered controls hit expected primitive1 with exact CPU T/bary references. Original TLAS is initial-build while centered controls refit: representation and history are confounded. No centering-only, driver-fault or production-workaround claim. Additional probes are held behind MAIN's lighting priority. |
| Historical adapter admission | Original LargeX control PID1796 failed before explicit device creation because historical UID68069/0 was absent. V2 pinned observed UID15696389/0, vendor4098/device30032, driver9007201352883061. LUID and driver are historical; enumerate and revalidate before any new run. Prior SDK receipt used driver9007201352878040. |
| BC tails | SDK94 full-mip `WHOLE_SIZE` route qualified through FACET: 25 original BC4/BC5/BC7 linear/sRGB textures,159mips, four ABBA runs,636 readbacks/715848032 bytes exact, four matching images, validation0, flush/oversized singleton/fault-recovery/retirement controls. Source receipt is now `C:/MIRA/Modules/LocalSources/FACET/Logs/StartupUpload/Native-20261020-Heavy-WholeMips/receipt.json`, SHA256 `478f77aa63e8e7239fa546d4e17d675466431091f36ee76485818f635fe22e66`. This chat rehashed all698 sealed files before relocation. Explicit2x2 failure exit2170 remains retained; provider normalization and arbitrary padded3D slices remain separate open work. Full application adoption/performance promotion were not established by that fixture. |
| BC top-level shapes | Separate unresolved SDK94 defect: requested17x9 compressed resources observed20x12 native dimensions. Tail readback success does not fix top-level logical/native shape mismatch. Preserve exact-shape guards and `Docs/BC_TEXTURE_DIMENSIONS.md` gates. |
| Batch BLAS indexing | Source-only deferred defect still present at `Source/D3D12/CommandBufferD3D12.hpp:2739`: inner geometry loop reads `desc.geometries[i]` rather than `[j]`. Current reviewed MIRA single-descriptor/single-geometry path does not reach it. Require a focused two-descriptor regression before enabling/fixing that batch path; do not attribute unrelated contact misses to it. |
| RmlUi/Metal | OpenRHI already has RmlUi6.3 public bridge/native renderer, not merely a future adapter. Windows UI readbacks were qualified; Metal shader-source/reflection coverage is recorded. Native Apple runtime/metallib/device coverage remains separate. The earlier generic chat answer suggesting a new OpenRHI adapter was corrected. |
| FSR first use | Read-only contract review found no demonstrated SDK blocker or dedicated public precompile API. Context create, destroy and dispatch share a global D3D12 FSR lock. Existing MIRA context creation precedes producer rendering. First `Show` includes acquire/UI/composition/waited submission/presentation; its51.6ms span does not identify FSR as the cause. MAIN explicitly paused warm-up implementation and diagnostics. |

## FSR candidate retained for later, currently held

If focused timing later justifies it, ordinary offscreen dispatch on the future published context is a source-supported candidate: initialize private matching color/depth/motion/reactive textures; retain the existing mode/resolution/flags/provider/sharpness; use caller-owned commands, COMMON/GENERAL/ALL and serialized queue ownership; wait the submission fence before retiring private inputs. Keep that context/output for real use. A separate background context can contend on the global SDK lock; recording against borrowed live renderer guides or a shared command allocator while loading UI records would race ownership.

The first real scene still needs explicit RESET_HISTORY, a jitter base set after preparatory completion, and separate preparation versus displayed-frame counters/active telemetry. `FsrPass::Record` only automatically resets while `completedFrames_` is zero, and existing activation initializes `fsrJitterBase` to zero. A preparatory dispatch changes both assumptions. Retain all preparation cost in first-lit/loading/presentation measurements; reset work may still cost. No implementation or runtime benefit was established. SDK94 and AMD/default settings remain unchanged.

## Evidence availability and validation performed for this handoff

Relocation preserves many historical receipts with old absolute `C:/OpenRHI`, `C:/FACET`, `C:/UVRT` paths. Treat them as provenance, not executable current launch manifests. MIRA's documented cleanup recycled generated builds/binaries and some generated fixture outputs; original receipts remain. Check each artifact's availability and identity before reproducing an old command. Do not rewrite failed/frozen receipts to present relocated files as originally captured paths.

This handoff performed read-only Git/root/provider checks, selected source-byte comparisons, SDK DLL hashes, migration source-hash verification and documentation inspection. Earlier chat changes passed their then-current source-boundary and diff checks; these are historical checks, not a fresh native qualification. Final handoff validation uses `git diff --check`, UTF-8/CRLF inspection, path existence and `python Scripts/Build/boundary.py --root .`. No compiler, dependency acquisition, device creation, GPU run, commit, push or cleanup is part of handoff preparation.

## Ordered next actions and acceptance gates

1. Confirm MAIN's current objective, roots, revisions, SDK/provider/shader identities and pending work. Re-read `.mira-local.json` or run MIRA's read-only module status. Keep development and integrated histories separate; preserve pre-existing edits. Gate: explicit selected-source/SDK association and no silent module pin change.
2. Advance MAIN's current first-lit/secondary-lighting critical path through existing public interfaces. The last request to this chat held FSR warming and extra LargeX diagnostics. Gate: first displayed scene lit, actual loading/activation/presentation deadline distribution retained, exact current-generation output and all original failed limits preserved.
3. Reconstruct generated outputs only when required through each owning pinned Xmake workflow. In development OpenRHI, `xmake f -P . -m release --build_tests=y --none=y -y`, `xmake build -P . -y OpenRHI`, `xmake test -P . "*/cpu"`, `xmake sdk -P .`; use this checkout as process working directory. `python Scripts/Build/bootstrap.py` locates the pinned Windows tool. Gate: affected CPU/native builds and SDK/export/license checks for the exact candidate; native tests only in MAIN's granted window.
4. Change OpenRHI implementation/SDK only for an actual supported-path blocker. Pair public facade/capabilities, Validation, native backends, source/export/ABI tests, SDK and consuming owner rebuilds. Gate: bounded unchanged-baseline failure where practical, candidate correctness/validation, lifetime and measured application acceptance; keep SDK94 baseline independently recoverable.
5. For eventual contact causality, obtain a matched world-coordinate identity-refit control and centered identity without refit before assigning centering as the cause. UVRT owns physical correctness and the reconstruction correction. Gate: original cases/bounds/source bytes/failures retained, strict original workload, native validation, correct current-AS signals. No blanket coordinate cutoff or driver attribution.
6. Extend production support only with UVRT's combined live contract and full denominator. Gate: applicable D01-D14 before/changed/recovered direct/shadow/diffuse/specular evidence, latency/frame/memory/queue limits, boundary pressure and recovery in the same declared envelope. Component proofs close zero production cells by themselves.

## Questions and risks for MIRA MAIN / Claude

- Which post-migration SDK/provider configuration is intended for the next application run: preserved SDK94 or a separately qualified new Xmake binary? Source selection alone does not answer this.
- Which latest MAIN startup and secondary-lighting receipts supersede the historical timings here? Do not reopen the retracted upstairs/second-floor geometry report; the user corrected it. Independent physical-lighting failures remain open.
- Integrated OpenRHI lacks some development-only receipt documentation; any promotion requires an explicit pin/update decision. Parent-owned manifests and consolidated handoff belong to MAIN.
- Old frozen launch scripts contain relocated absolute paths, old adapter LUIDs and potentially cleaned binaries. Rebuild/reseal a new experiment rather than treating stale manifests as launch-ready.
- BC full-mip success is available without SDK replacement. Explicit compressed-region normalization, top-level odd BC shape admission and padded3D slices remain distinct issues.
- Queue/fence ownership, async preparation publication and descriptor-record retains are the integration risks. Thread-safe creation/global SDK locks do not prove independent render progress or safe shared command recording.

No native child or GPU lease is held by this chat. This document is the only intended uncommitted handoff edit; MAIN owns consolidation and subsequent sequencing.
