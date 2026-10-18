#include "OpenRHI.h"
#ifdef _WIN32
#include "OpenRHIAgilitySDK.h"
#endif
#include "Extensions/OpenRHIDeviceCreation.h"
#include "SdkShader.h"
#include <cstdio>
int main() {
    using namespace openrhi;
    DeviceCreationDesc create = {};
#ifdef _WIN32
    create.graphicsAPI = GraphicsAPI::D3D12;
#elif defined(__APPLE__)
    create.graphicsAPI = GraphicsAPI::METAL;
#else
    create.graphicsAPI = GraphicsAPI::VK;
#endif
    create.enableOpenRHIValidation = true;
    Device* device = nullptr;
    if (openrhiCreateDevice(create, device) != Result::SUCCESS) return 1;
    CoreInterface core = {};
    if (openrhiGetInterface(*device, OPENRHI_INTERFACE(CoreInterface), &core) != Result::SUCCESS) return 2;
    PipelineLayoutDesc layoutDesc = {}; layoutDesc.shaderStages = StageBits::COMPUTE_SHADER;
    PipelineLayout* layout = nullptr;
    if (core.CreatePipelineLayout(*device, layoutDesc, layout) != Result::SUCCESS) return 3;
    ComputePipelineDesc pipelineDesc = {}; pipelineDesc.pipelineLayout = layout; pipelineDesc.shader = g_SdkShader_desc;
    Pipeline* pipeline = nullptr;
    if (core.CreateComputePipeline(*device, pipelineDesc, pipeline) != Result::SUCCESS) return 4;
    core.DestroyPipeline(pipeline); core.DestroyPipelineLayout(layout); openrhiDestroyDevice(device);
    std::puts("OPENRHI_SLANG_SDK_CONSUMER_PASS");
}
