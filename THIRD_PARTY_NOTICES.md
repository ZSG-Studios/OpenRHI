# Third-party notices

## OpenRHI first-party work

OpenRHI's first-party code and documentation are licensed under the MIT terms in [LICENSE.txt](LICENSE.txt), with the ZSG-Studios contributors' notice. Source-derived code preserves its original copyrights and applicable license notices. Dependencies, compiler distributions and optional provider runtimes retain their own licenses and packaged attribution as recorded below.

## NVIDIA NRI

OpenRHI includes code derived from NVIDIA NRI, distributed under the MIT license.

- Upstream: https://github.com/NVIDIA-RTX/NRI
- Recorded source commit: `8ee40ca5c2efc3c8c9c980dc6d66c881fbc5ec77`
- Per-commit integration and verification: [Docs/UPSTREAM.md](Docs/UPSTREAM.md)
- Original copyright notices remain in the derived source files

The applicable license is reproduced below and retained in LICENSE.txt.

```text
Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
```

## Additional dependencies

Dependency-provided licenses accompany packaged native libraries in ThirdPartyLicenses. NVIDIA Image Scaling retains its source license and algorithm attribution. Compiler and dependency distributions retain their respective license terms.

## UI public-interface integrations

The OpenRHI-authored RmlUi bridge uses the unmodified public API of RmlUi 6.3, commit `ba95ffe8bfb6370efb2cdcca927eaad4710c5413` of https://github.com/mikke89/RmlUi. RmlUi is MIT licensed; its original `LICENSE.txt` remains in the fetched test dependency. The bridge does not copy or modify RmlUi implementation. Applications that link or deploy RmlUi preserve its original license alongside their RmlUi distribution.

The ImGui renderer contract is qualified with Dear ImGui 1.92.9, commit `01380c579715e62fb9a8d6ec0502c4ea83bfde6e` of https://github.com/ocornut/imgui. Dear ImGui is MIT licensed; the unmodified test dependency retains its `LICENSE.txt`. OpenRHI's renderer retains the upstream NVIDIA source attribution. Application-owned ImGui distributions retain Dear ImGui's original license. Neither UI core is linked into the OpenRHI runtime; applications link their context-owning UI cores.

## Qualcomm Adreno Neural Fusion SDK

The optional Android Vulkan adapter uses the unmodified public ANF SDK 0.43.0 from Snapdragon Game Studios, commit `a144fbab47ad586adf64c5a1574be73064550f1e` of https://github.com/SnapdragonGameStudios/adreno-neural-fusion.

Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved. Public headers carry BSD-3-Clause terms in `LICENSE-BSD-3-Clause.txt`; the binary carries the separate terms in `LICENSE.txt`. Enabled builds copy both original files into `ThirdPartyLicenses/anf` alongside `libanf.so`. The device-lab APK retains both licenses in its assets.

## Neural lighting and latency providers

- AMD FSR SDK 2.3.0, commit `60f4ea81909200d8542eca14dccb2628b763a9a3`: Frame Generation API 4.0.1 / D3D12 proxy API 3.1.7, Ray Regeneration 1.2.0 and radiance caching 0.9.0 preview use the original public `ffx_framegeneration.h`, `ffx_api_framegeneration_dx12.h`, `ffx_denoiser.h`, `ffx_radiancecache.h` and signed provider DLLs. Original AMD SDK licenses accompany enabled builds in `ThirdPartyLicenses/fsr`.
- NVIDIA NRC 0.16, commit `7de30e4eb2a1a27118eb9a6acf5f715d757240c0`: optional native D3D12 adapter uses the unchanged public headers, import library, NRC runtime and CUDA runtime dependencies. The NVIDIA RTX SDK license remains applicable and accompanies enabled builds in `ThirdPartyLicenses/nrc/License.md`; it is independent of OpenRHI's MIT license.
- AMD Anti-Lag 2 SDK, commit `390aa4a8c8655d0ae6e90079db2c85e103a96da3`: optional D3D12 adapter calls the unchanged public header API. Copyright 2024 Advanced Micro Devices, Inc.; MIT license accompanies enabled builds in `ThirdPartyLicenses/antilag2`.
- NVIDIA Reflex uses the existing pinned NVAPI dependency on D3D12 and the native Vulkan `VK_NV_low_latency2` public API on Linux. Vendor sources remain unchanged.
