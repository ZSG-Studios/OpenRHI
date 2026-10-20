struct HeapIndices { uint first; uint second; };
ConstantBuffer<HeapIndices> indices : register(b0, space1);
[shader("compute")]
[numthreads(64, 1, 1)]
void descriptorMain(uint3 tid : SV_DispatchThreadID) {
    uint slot = ((tid.x & 1) != 0) ? indices.second : indices.first;
    RWStructuredBuffer<uint> target = ResourceDescriptorHeap[NonUniformResourceIndex(slot)];
    target[tid.x >> 1] = tid.x * 3 + 7;
}
