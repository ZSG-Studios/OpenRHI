#include "OpenRHI.h"

#include "Extensions/OpenRHIDeviceCreation.h"

#include <fcntl.h>
#include <jni.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

int OpenRHIQualificationMain(int, char**);
int OpenRHISmokeMain(int, char**);
int OpenRHIPolicyMain();
int OpenRHIPublicMain();
int ANFCompatibilityMain(int, char**);
int TestAnfDispatch();

static void Check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

static std::string Read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);

    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

static int Invoke(int (*mainFunction)(int, char**), std::vector<std::string> arguments) {
    std::vector<char*> pointers;

    for (auto& argument : arguments)
        pointers.push_back(argument.data());

    return mainFunction(int(pointers.size()), pointers.data());
}

struct CacheFixture {
    openrhi::Device* device = nullptr;
    openrhi::CoreInterface core = {};
    openrhi::PipelineLayout* layout = nullptr;
    openrhi::Pipeline* cold = nullptr;
    openrhi::Pipeline* warm = nullptr;
    openrhi::PipelineCache* cache = nullptr;
    openrhi::PipelineCache* restored = nullptr;
    openrhi::CommandAllocator* allocator = nullptr;
    openrhi::CommandBuffer* command = nullptr;
    openrhi::Fence* fence = nullptr;
    uint32_t errors = 0;

    ~CacheFixture() {
        if (!device)
            return;

        if (core.DeviceWaitIdle) {
            core.DeviceWaitIdle(device);
            if (fence)
                core.DestroyFence(fence);
            if (command)
                core.DestroyCommandBuffer(command);
            if (allocator)
                core.DestroyCommandAllocator(allocator);
            if (warm)
                core.DestroyPipeline(warm);
            if (cold)
                core.DestroyPipeline(cold);
            if (restored)
                core.DestroyPipelineCache(restored);
            if (cache)
                core.DestroyPipelineCache(cache);
            if (layout)
                core.DestroyPipelineLayout(layout);
        }
        openrhi::openrhiDestroyDevice(device);
    }
};

static void OPENRHI_CALL CacheMessage(openrhi::Message type, const char*, uint32_t, const char* message, void* context) {
    if (type == openrhi::Message::ERROR)
        (*static_cast<uint32_t*>(context))++;
    std::fprintf(stderr, "%s\n", message);
}

static void OPENRHI_CALL CacheNoAbort(void*) {
}

static int TestCache(const std::string& directory) {
    using namespace openrhi;
    CacheFixture fixture;
    DeviceCreationDesc creation = {};
    creation.graphicsAPI = GraphicsAPI::VK;
    creation.enableOpenRHIValidation = true;
    creation.enableGraphicsAPIValidation = true;
    creation.callbackInterface = {CacheMessage, CacheNoAbort, &fixture.errors};
    Check(openrhiCreateDevice(creation, fixture.device) == Result::SUCCESS, "cache device creation");
    auto& core = fixture.core;
    Check(openrhiGetInterface(*fixture.device, OPENRHI_INTERFACE(CoreInterface), &core) == Result::SUCCESS, "cache core interface");
    const auto& desc = core.GetDeviceDesc(*fixture.device);
    std::printf("DEVICE %s cache=%d cacheControl=%d\n", desc.adapterDesc.name, desc.features.pipelineCache, desc.features.pipelineCacheControl);

    if (!desc.features.pipelineCache)
        return 77;

    PipelineLayoutDesc layout = {};
    layout.shaderStages = StageBits::COMPUTE_SHADER;
    Check(core.CreatePipelineLayout(*fixture.device, layout, fixture.layout) == Result::SUCCESS, "cache layout");
    Check(core.CreatePipelineCache(*fixture.device, {}, fixture.cache) == Result::SUCCESS, "empty cache");
    std::string bytes = Read(directory + "/CacheProbe.spirv");
    Check(!bytes.empty(), "cache shader asset");
    ComputePipelineDesc pipeline = {};
    pipeline.pipelineLayout = fixture.layout;
    pipeline.shader = {StageBits::COMPUTE_SHADER, bytes.data(), uint64_t(bytes.size()), "cacheMain"};
    pipeline.cache = fixture.cache;
    Check(core.CreateComputePipeline(*fixture.device, pipeline, fixture.cold) == Result::SUCCESS, "cold pipeline");
    uint64_t size = 0;
    Check(core.GetPipelineCacheData(*fixture.cache, nullptr, size) == Result::SUCCESS && size > 0, "cache size");
    std::vector<uint8_t> blob(size);
    Check(core.GetPipelineCacheData(*fixture.cache, blob.data(), size) == Result::SUCCESS, "cache serialization");
    std::ofstream saved(directory + "/pipeline-cache.bin", std::ios::binary);
    saved.write(reinterpret_cast<const char*>(blob.data()), size);
    saved.close();
    std::string loaded = Read(directory + "/pipeline-cache.bin");
    Check(loaded.size() == size, "cache file roundtrip");
    PipelineCacheDesc restore = {loaded.data(), uint64_t(loaded.size())};
    Check(core.CreatePipelineCache(*fixture.device, restore, fixture.restored) == Result::SUCCESS, "cache restore");
    pipeline.cache = fixture.restored;

    if (desc.features.pipelineCacheControl)
        pipeline.flags = ComputePipelineBits::FAIL_ON_CACHE_MISS;
    Check(core.CreateComputePipeline(*fixture.device, pipeline, fixture.warm) == Result::SUCCESS, "restored pipeline cache hit");
    Queue* queue = nullptr;
    Check(core.GetQueue(*fixture.device, QueueType::GRAPHICS, 0, queue) == Result::SUCCESS, "cache queue");
    Check(core.CreateCommandAllocator(*queue, fixture.allocator) == Result::SUCCESS, "cache allocator");
    Check(core.CreateCommandBuffer(*fixture.allocator, fixture.command) == Result::SUCCESS, "cache command");
    Check(core.CreateFence(*fixture.device, 0, fixture.fence) == Result::SUCCESS, "cache fence");
    Check(core.BeginCommandBuffer(*fixture.command, nullptr) == Result::SUCCESS, "cache begin");
    core.CmdSetPipelineLayout(*fixture.command, BindPoint::COMPUTE, *fixture.layout);
    core.CmdSetPipeline(*fixture.command, *fixture.warm);
    core.CmdDispatch(*fixture.command, {1, 1, 1});
    Check(core.EndCommandBuffer(*fixture.command) == Result::SUCCESS, "cache end");
    FenceSubmitDesc signal = {fixture.fence, 1, StageBits::ALL};
    QueueSubmitDesc submission = {};
    submission.commandBuffers = &fixture.command;
    submission.commandBufferNum = 1;
    submission.signalFences = &signal;
    submission.signalFenceNum = 1;
    Check(core.QueueSubmit(*queue, submission) == Result::SUCCESS, "cached pipeline submit");
    core.Wait(*fixture.fence, 1);
    Check(core.GetFenceValue(*fixture.fence) >= 1, "cached pipeline fence");
    core.DeviceWaitIdle(fixture.device);
    Check(fixture.errors == 0, "cache validation errors");
    std::printf("CACHE bytes=%llu persisted=1 restored=1 dispatched=1 fail_on_miss=%d\n", (unsigned long long)size, desc.features.pipelineCacheControl);

    return 0;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_openrhi_nativelab_MainActivity_runNative(JNIEnv* env, jclass, jstring files, jstring libraries) {
    const char* path = env->GetStringUTFChars(files, nullptr);
    const std::string directory(path);
    env->ReleaseStringUTFChars(files, path);
    path = env->GetStringUTFChars(libraries, nullptr);
    const std::string nativeDirectory(path);
    env->ReleaseStringUTFChars(libraries, path);
    setenv("XDG_CACHE_HOME", directory.c_str(), 1);
    setenv("TMPDIR", directory.c_str(), 1);
    std::string summary = "Build: " LAB_CONFIGURATION " / ARM64\n\n";
    int output = open((directory + "/native.log").c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    int oldOut = dup(STDOUT_FILENO), oldErr = dup(STDERR_FILENO);
    fflush(nullptr);
    dup2(output, STDOUT_FILENO);
    dup2(output, STDERR_FILENO);
    close(output);
    auto test = [&](const char* name, const std::function<int()>& function) {
        std::printf("\nTEST_BEGIN %s\n", name);
        fflush(nullptr);
        int result = 1;

        try {
            result = function();
        } catch (const std::exception& error) {
            std::printf("EXCEPTION %s\n", error.what());
        }
        const char* status = result == 0 ? "PASS" : (result == 77 ? "SKIP" : "FAIL");
        summary += std::string(status) + "  " + name + "\n";
        std::printf("TEST_END %s result=%d\n", name, result);
        fflush(nullptr);
    };
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    bool validationAvailable = false;

    for (const auto& layer : layers)
        validationAvailable |= std::string(layer.layerName) == "VK_LAYER_KHRONOS_validation";
    summary += std::string("Vulkan validation: ") + (validationAvailable ? "available; requested\n\n" : "unavailable\n\n");
    test("C/C++ public API", [] { return OpenRHIPublicMain(); });
    test("Device policy", [] { return OpenRHIPolicyMain(); });
    test("Vulkan queue + fence", [] { return Invoke(OpenRHISmokeMain, {"smoke", "VK"}); });
    test("GPU copy / compute / NIS / draw", [&] {
        return Invoke(OpenRHIQualificationMain, {"resources", "--api", "VK", "--iterations", "8", "--native-validation", "--nis", "--shader", directory + "/DescriptorCompute.spirv", "--vs", directory + "/IndirectDraw.vs.spirv", "--ps", directory + "/IndirectDraw.ps.spirv"});
    });
    test("Pipeline cache save / load / dispatch", [&] { return TestCache(directory); });
    test("Task / mesh rendering", [&] {
        return Invoke(OpenRHIQualificationMain, {"mesh", "--api", "VK", "--iterations", "2", "--native-validation", "--mesh", "--vs", directory + "/MeshDraw.spirv", "--task", directory + "/TaskDraw.spirv", "--ps", directory + "/IndirectDraw.ps.spirv"});
    });
    summary += "\nVendor SDK check (separate):\n";
    test("ANF SDK support queries", [&] { return Invoke(ANFCompatibilityMain, {"anf", nativeDirectory + "/libanf.so"}); });
    summary += "\nOpenRHI ANF integration:\n";
    int anfResult = 1;
    test("ANF SR dispatch + readback (3 frames)", [&] { return anfResult = TestAnfDispatch(); });

    if (anfResult == 77)
        summary += "ANF: driver rejects SR technique creation.\nAdapter returned UNSUPPORTED; no dispatch ran.\n";
    summary += "\nVendor RR / radiance caching:\nunavailable on this Android SDK provider.\n\nRaw results: native.log\nThese are short correctness tests.\n";
    fflush(nullptr);
    dup2(oldOut, STDOUT_FILENO);
    dup2(oldErr, STDERR_FILENO);
    close(oldOut);
    close(oldErr);

    return env->NewStringUTF(summary.c_str());
}
