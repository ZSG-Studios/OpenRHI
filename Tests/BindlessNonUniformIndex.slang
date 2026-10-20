// CPU compiler diagnostic: the stored-intrinsic variant is never dispatched
cbuffer HeapIndices : register(b0, space1) {
    uint count;
    uint input0;
    uint input1;
    uint output0;
    uint output1;
};
[shader("compute")]
[numthreads(64, 1, 1)]
void nonUniformIndexMain(uint3 tid : SV_DispatchThreadID) {
    uint index = tid.x;
    if (index >= count)
        return;
    uint lane = index & 1;
#if OPENRHI_TEST_STORED_NONUNIFORM
    uint source = NonUniformResourceIndex((lane == 0) ? input0 : input1);
    uint destination = NonUniformResourceIndex((lane == 0) ? output0 : output1);
    StructuredBuffer<uint4> inputs = ResourceDescriptorHeap[source];
    RWStructuredBuffer<uint2> outputs = ResourceDescriptorHeap[destination];
#else
    uint source = (lane == 0) ? input0 : input1;
    uint destination = (lane == 0) ? output0 : output1;
    StructuredBuffer<uint4> inputs = ResourceDescriptorHeap[NonUniformResourceIndex(source)];
    RWStructuredBuffer<uint2> outputs = ResourceDescriptorHeap[NonUniformResourceIndex(destination)];
#endif
    uint4 query = inputs[index];
    outputs[index] = uint2(query.x + 1, query.y + 2);
}
