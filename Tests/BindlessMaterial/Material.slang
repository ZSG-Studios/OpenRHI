struct SampleCase {
    uint textureIndex, samplerIndex, operation, reserved;
    float2 uv;
    float lod, padding;
    float2 dx, dy;
};
struct SampleResult { float4 color; uint width, height, levels, reserved; };
struct Root { uint casesIndex, outputIndex, firstCase, caseCount; };
ConstantBuffer<Root> root : register(b0, space1);
#if OPENRHI_MATERIAL_CONVENTIONAL
StructuredBuffer<SampleCase> cases : register(t0);
RWStructuredBuffer<SampleResult> results : register(u1);
Texture2D<float4> source : register(t2);
SamplerState sampling : register(s3);
#endif
[shader("compute")]
[numthreads(64, 1, 1)]
void materialMain(uint lane : SV_DispatchThreadID) {
    if (lane >= root.caseCount) return;
#if !OPENRHI_MATERIAL_CONVENTIONAL
    StructuredBuffer<SampleCase> cases = ResourceDescriptorHeap[root.casesIndex];
    RWStructuredBuffer<SampleResult> results = ResourceDescriptorHeap[root.outputIndex];
#endif
    uint index = root.firstCase + lane;
    SampleCase c = cases[index];
#if !OPENRHI_MATERIAL_CONVENTIONAL
    Texture2D<float4> source = ResourceDescriptorHeap[NonUniformResourceIndex(c.textureIndex)];
    SamplerState sampling = SamplerDescriptorHeap[NonUniformResourceIndex(c.samplerIndex)];
#endif
    SampleResult result = (SampleResult)0;
    source.GetDimensions(0, result.width, result.height, result.levels);
    if (c.operation == 2) result.color = source.SampleGrad(sampling, c.uv, c.dx, c.dy);
    else result.color = source.SampleLevel(sampling, c.uv, c.operation == 0 ? 0 : c.lod);
    results[index] = result;
}
