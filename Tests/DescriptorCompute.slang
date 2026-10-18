#include "OpenRHI.slang"
#if OPENRHI_TARGET_METAL
struct ComputeRoot { OpenRhiStorageBufferEntry* targets; };
[[vk::binding(2, 0)]] ConstantBuffer<ComputeRoot> root : register(b2);
#else
[[vk::binding(0, 0)]] RWStructuredBuffer<uint> targets[2] : register(u0, space0);
#endif
[shader("compute")]
[numthreads(64, 1, 1)]
void descriptorMain(uint3 tid : SV_DispatchThreadID) {
#if OPENRHI_TARGET_METAL
    root.targets[tid.x & 1].data[tid.x >> 1] = tid.x * 3 + 7;
#else
    targets[NonUniformResourceIndex(tid.x & 1)][tid.x >> 1] = tid.x * 3 + 7;
#endif
}
