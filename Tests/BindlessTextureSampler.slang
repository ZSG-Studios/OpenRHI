struct HeapIndices {
    uint firstTexture;
    uint secondTexture;
    uint firstSampler;
    uint secondSampler;
    uint outputTexture;
};
ConstantBuffer<HeapIndices> indices : register(b0, space1);
[shader("compute")]
[numthreads(8, 8, 1)]
void textureSamplerMain(uint3 tid : SV_DispatchThreadID) {
    uint textureIndex = ((tid.x & 1) != 0) ? indices.secondTexture : indices.firstTexture;
    uint samplerIndex = ((tid.x & 2) != 0) ? indices.secondSampler : indices.firstSampler;
    Texture2D<float4> source = ResourceDescriptorHeap[NonUniformResourceIndex(textureIndex)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(samplerIndex)];
    RWTexture2D<float4> output = ResourceDescriptorHeap[indices.outputTexture];
    output[tid.xy] = source.SampleLevel(samplerState, float2(1.25, 0.5), 0);
}
