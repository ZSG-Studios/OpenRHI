// Verify enabled/disabled UI interface discovery through NONE and Validation
#include "OpenRHI.h"

#include <cstdio>
#include <initializer_list>
#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIRmlUi.h"
#include "Extensions/OpenRHIOptions.hpp"

using namespace openrhi;

int main() {
    for (bool validation : {false, true}) {
        DeviceCreationDesc creation = {};
        creation.graphicsAPI = GraphicsAPI::NONE;
        creation.enableOpenRHIValidation = validation;
        Device* device = nullptr;

        if (openrhiCreateDevice(creation, device) != Result::SUCCESS)
            return 1;

        RmlUiInterface table = {};
        Result result = openrhiGetInterface(*device, OPENRHI_INTERFACE(RmlUiInterface), &table);
        bool ok = result == (OPENRHI_TEST_RMLUI ? Result::SUCCESS : Result::UNSUPPORTED);
        RenderOptionsSnapshot options = {};
        ok = ok && QueryOpenRHIRenderOptions(*device, options) == Result::SUCCESS && !options.nativeDevice;
        ok = ok && GetOpenRHIRenderOptionDescs()[size_t(RenderOption::FRAME_GENERATION)].publicExecutionPath;
        ok = ok && options.options[size_t(RenderOption::FRAME_GENERATION)].interfaceResult == Result::UNSUPPORTED;

        for (const auto& option : options.options)
            ok = ok && !option.upscalerReportedSupported;

        if (OPENRHI_TEST_RMLUI && result == Result::SUCCESS) {
            auto caps = table.GetRmlUiCapabilities(*device);
            ok = ok && !caps.nativeRendering && caps.geometry && caps.textures && caps.scissor && caps.transforms && !caps.clipMasks && !caps.layers && !caps.filters && !caps.customShaders;
            RmlUi* ui = nullptr;
            ok = ok && table.CreateRmlUi(*device, {Format::RGBA8_UNORM, 2}, ui) == Result::SUCCESS && ui;

            if (ui) {
                CoreInterface core = {};
                ok = ok && openrhiGetInterface(*device, OPENRHI_INTERFACE(CoreInterface), &core) == Result::SUCCESS;
                core.SetDebugName(ui, "RmlUi lifecycle test");
                table.ResetRmlUi(*ui);
                table.DestroyRmlUi(ui);
            }
        } else
            ok = ok && !table.CreateRmlUi && !table.CmdDrawRmlUi;

        openrhiDestroyDevice(device);

        if (!ok)
            return 2;
    }

    std::puts("RmlUi availability, NONE capability boundary and Validation lifecycle: PASS");

    return 0;
}
