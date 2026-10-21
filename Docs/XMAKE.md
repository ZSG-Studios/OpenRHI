# OpenRHI Xmake build

All owned build graphs use Xmake3.1.2. `xmake.lua` declares the native library and platform/provider feature gates; `Xmake/tests.lua` declares tests and original toolkit source libraries. Provider packages stay unmodified and retain their version/revision pins in `Xmake/dependencies.json`.

From the owning checkout, configure with `xmake f -P . -m release --build_tests=y --none=y -y`, build with `xmake build -P . -y OpenRHI`, run host contracts with `xmake test -P . "*/cpu"`, and package with `xmake sdk -P .`. Native GPU test targets require their matching hardware and a coordinated GPU window. `python Scripts/Build/bootstrap.py` prints the pinned, SHA-256-verified Windows portable executable; other hosts supply Xmake3.1.2.

| Option | Ownership |
| --- | --- |
| `--d3d12=y`, `--vk=y`, `--metal=y`, `--none=y` | Native platform and explicit test backend |
| `--static_library=y`, `--validation=y`, `--debug_names=y` | Library kind, validation and diagnostics |
| `--imgui=y`, `--rmlui=y`, `--nis=y` | Maintained UI renderers and NIS kernels |
| `--agility=y`, `--nvapi=y`, `--amdags=y`, `--nvtx=y` | Native SDK/provider gates |
| `--ffx=y`, `--frame_generation=y`, `--antilag2=y` | AMD rendering/latency providers |
| `--ngx=y`, `--xess=y`, `--nrc=y` | Optional NVIDIA/Intel providers |
| `--anf=y --anf_root=PATH` | Admitted ANF0.43.0 Android provider |
| `--build_tests=y` | Public ABI, host, toolkit and native test targets |
| `--apple_sdk=iphoneos|iphonesimulator|macosx` | Shader native assembly SDK |

Windows defaults preserve D3D12, Agility, NVAPI, AGS, NVTX, FSR, Anti-Lag2, frame generation, ImGui and RmlUi where their platform/architecture gates apply. Linux and Android use Vulkan; Apple uses Metal4. Android uses `-p android -a arm64-v8a --ndk=PATH --ndk_sdkver=33`; iOS simulator adds `-p iphoneos --appledev=simulator --apple_sdk=iphonesimulator`. Cross-platform configuration is distinct from measured device qualification.

Native outputs use `Build/Objects`, `Build/Generated` and `Bin/<platform>/<architecture>/<mode>`. The SDK uses `Lib/Debug` and `Lib/Release`, public headers, native runtimes/licenses and the public Xmake shader helper. Package one target platform/architecture per SDK directory. Current qualified consumer SDK native bytes are preserved while new Xmake outputs are qualified separately.

The migration ran Windows NONE/host checks and full-feature D3D12 compilation plus existing D3D12 smoke/adapter checks. Independent SDK optional-runtime packaging and eight-process cold dependency acquisition controls pass. Original failure logs and corrected outputs live in `Logs/XmakeMigration`. Additional platform and advanced native/runtime cases retain their own available-host gates.
