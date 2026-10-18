# Integrating OpenRHI API revision 1

Rebuild consumers against the OpenRHI headers and binaries. Third-party notices are provided in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

| Integration | OpenRHI contract |
|---|---|
| Public headers | `OpenRHI.h`, `OpenRHIDescs.h`, and `Extensions/OpenRHI*.h` |
| C++ namespace | `openrhi` |
| C exports | `openrhiCreateDevice`, `openrhiGetInterface`, and the `openrhi*` API |
| C facade | `OpenRhiDevice`, `OpenRhiStruct`, and the `OpenRhi*` type macros |
| API and build macros | `OPENRHI_*` |
| Validation | `enableOpenRHIValidation` |
| CMake target and library | `OpenRHI` |
| Shader source and include | `.slang` and `OpenRHI.slang` |
| SDK folder | `_OpenRHI_SDK` |

Windows applications use D3D12; Linux and Android use Vulkan; Apple applications use native Metal 4. Native wrapper entrypoints follow these platform boundaries. Enable `OPENRHI_ENABLE_NONE_SUPPORT` explicitly for dummy-device tests.

Pass an enumerated adapter to `DeviceCreationDesc::adapterDesc` for explicit selection. A failed explicit selection returns its compatibility report and a null device. Automatic selection tries eligible adapters in preference order: discrete, available memory budget, then ascending UID.

Use `openrhiGetPlatformRequirements` for the mandatory baseline. `DeviceRequirements` adds requirements. `openrhiQueryAdapterSupport` performs an explicit temporary-device probe; `openrhiEvaluateDeviceSupport` evaluates an existing capability snapshot without creating a device. Supply `compatibilityReport` during creation for missing-feature, shader-model, memory-budget, and backend diagnostics.

`openrhiGetDeviceProfile` returns an immutable creation-time profile. `GENERIC` explicitly requests the conservative policy. `AUTOMATIC` recognizes supported identity records and currently selects conservative policies until qualification permits tuning. Queue and wave values are recommendations for explicit application choices.

For imported Vulkan devices, provide Vulkan 1.3+ and an `enabledCapabilities` mask describing the baseline features actually enabled on the logical device. OpenRHI intersects this declaration with physical-device support. Querying physical support alone does not establish that the imported logical device enabled a feature.

Pipeline and shader caches belong to the application. Invalidate them across the rename and include backend, device/driver identity, compiler, shader contents, and layout ABI in cache keys.

Format conversion entrypoints use `openrhiConvertOpenRHIFormatToDXGI` and `openrhiConvertOpenRHIFormatToVK`, paired with `openrhiConvertDXGIFormatToOpenRHI` and `openrhiConvertVKFormatToOpenRHI`. Rebuild against these spellings when updating an early fork checkout.

## Slang shader integration

Use `.slang` sources and `OpenRHI.slang`, compile through `CMake/OpenRHISlang.cmake`, and consume its native binary and reflection-derived `ShaderDesc`. Rebuild shader binaries and pipeline caches for this API revision. Native Metal pipeline layouts use the root data and typed resource-table ABI in [SHADERS.md](SHADERS.md). The Metal wrapper exposes resource and fence wrapping; shader compilation is explicit in the application build.
