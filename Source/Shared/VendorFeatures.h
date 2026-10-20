#pragma once

namespace openrhi {

Result FillVendorFeaturesInterface(FidelityFXInterface& table);
Result FillVendorFeaturesInterface(NeuralRadianceCacheInterface& table);
Result FillVendorFeaturesInterface(AntiLagInterface& table);
Result FillFrameGenerationInterface(FrameGenerationInterface& table);

} // namespace openrhi
