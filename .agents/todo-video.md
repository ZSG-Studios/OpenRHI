# OpenRHI Video TODO

## D3D12 Distinct Output/DPB Implementation

- Implement distinct D3D12 decode output and DPB setup pictures using `D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS::ConversionArguments`.
- Query `D3D12_FEATURE_VIDEO_DECODE_CONVERSION_SUPPORT`, define how decode/output color spaces are supplied, and advertise `decodeDpbAndOutputDistinct` only when the requested conversion is supported.

## AV1 Encode Parity

- Decide whether Vulkan AV1 segmentation should remain explicitly unsupported or be implemented when driver capabilities make it viable.
- Represent and implement Vulkan CDEF, restoration, screen-content sequence options, optional picture sub-descriptors, and render sizes that differ from coded size.
- Reconcile Vulkan and D3D12 AV1 segmentation behavior so equivalent neutral descriptors produce equivalent results where supported.

## Recommended Sample Coverage

- Exercise coincident and distinct decode output/DPB modes, including capability gating.
- Exercise H.264/H.265 decoded and reference-picture state transitions on Vulkan and D3D12.
- Exercise AV1 segmentation, CDEF, restoration, screen-content options, optional picture data, and render sizes distinct from coded sizes.
- Exercise encode feedback with a nonzero destination bitstream offset through explicit command-buffer resolves and a copy to host readback; use the advertised resolve queue and resolved-metadata state, and respect the advertised maximum pending-feedback count.
- Exercise decode bitstream transitions and decoded-picture readback after video-queue work on every supported backend.
