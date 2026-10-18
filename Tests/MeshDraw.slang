struct Payload { uint value; };
struct Vertex { float4 position : SV_Position; };
[shader("amplification")][numthreads(1,1,1)]
void taskMain(uint3 tid : SV_DispatchThreadID) {
    Payload payload; payload.value = tid.x;
    DispatchMesh(1, 1, 1, payload);
}
[shader("mesh")][outputtopology("triangle")][numthreads(1,1,1)]
void meshMain(in payload Payload input, out vertices Vertex output[3], out indices uint3 triangles[1]) {
    SetMeshOutputCounts(3, 1);
    float2 positions[3] = {float2(-1,-1),float2(3,-1),float2(-1,3)};
    for(uint i=0;i<3;i++) {
        Vertex vertex;
        vertex.position = float4(positions[i], 0.5 + float(input.value) * 0.01, 1);
        output[i] = vertex;
    }
    triangles[0] = uint3(0,1,2);
}
