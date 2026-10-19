#include "OpenRHI.h"

#include <cstdio>
#include <initializer_list>
#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIVendorFeatures.h"

using namespace openrhi;
#define CHECK(x) \
    do { \
        if (!(x)) { \
            std::printf("FAIL line %d: %s\n", __LINE__, #x); \
            return 1; \
        } \
    } while (false)

int main() {
    for (bool validation : {false, true}) {
        DeviceCreationDesc desc = {};
        desc.graphicsAPI = GraphicsAPI::NONE;
        desc.enableOpenRHIValidation = validation;
        Device* device = nullptr;
        CHECK(openrhiCreateDevice(desc, device) == Result::SUCCESS);
        FidelityFXInterface ffx = {};
        auto result = openrhiGetInterface(*device, OPENRHI_INTERFACE(FidelityFXInterface), &ffx);
        CHECK(result == (OPENRHI_TEST_FFX ? Result::SUCCESS : Result::UNSUPPORTED));
        if (OPENRHI_TEST_FFX) {
            FidelityFXContext* context = (FidelityFXContext*)1;
            FidelityFXDesc create = {};
            CHECK(ffx.CreateFidelityFXContext(*device, create, context) == Result::UNSUPPORTED && !context);
        }
        NeuralRadianceCacheInterface nrc = {};
        result = openrhiGetInterface(*device, OPENRHI_INTERFACE(NeuralRadianceCacheInterface), &nrc);
        CHECK(result == (OPENRHI_TEST_NRC ? Result::SUCCESS : Result::UNSUPPORTED));
        if (OPENRHI_TEST_NRC) {
            NeuralRadianceCache* context = (NeuralRadianceCache*)1;
            uint64_t storage[64] = {};
            NeuralRadianceCacheDesc create = {storage, storage, nullptr, nullptr};
            CHECK(nrc.CreateNeuralRadianceCache(*device, create, context) == Result::UNSUPPORTED && !context);
            CHECK(nrc.GetNeuralRadianceCacheAllocationInfo(*device, create, storage) == Result::UNSUPPORTED);
        }
        AntiLagInterface anti = {};
        result = openrhiGetInterface(*device, OPENRHI_INTERFACE(AntiLagInterface), &anti);
        CHECK(result == (OPENRHI_TEST_ANTILAG ? Result::SUCCESS : Result::UNSUPPORTED));
        if (OPENRHI_TEST_ANTILAG) {
            AntiLagContext* context = (AntiLagContext*)1;
            CHECK(anti.CreateAntiLagContext(*device, context) == Result::UNSUPPORTED && !context);
        }
        openrhiDestroyDevice(device);
    }
    std::puts("Vendor adapter availability and NONE null-output rejection: PASS (Validation off/on)");
    return 0;
}
