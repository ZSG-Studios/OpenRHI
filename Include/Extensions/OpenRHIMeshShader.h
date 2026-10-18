// © 2021 NVIDIA Corporation

// Goal: mesh shaders
// https://www.khronos.org/blog/mesh-shading-for-vulkan
// https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html

#pragma once

#define OPENRHI_MESH_SHADER_H 1

OpenRhiNamespaceBegin

OpenRhiStruct(DrawMeshTasksDesc) {
    uint32_t x, y, z;
};

// Threadsafe: no
OpenRhiStruct(MeshShaderInterface) {
    // Command buffer
    // {
        // Draw
        void    (OPENRHI_CALL *CmdDrawMeshTasks)            (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(DrawMeshTasksDesc) drawMeshTasksDesc);
        void    (OPENRHI_CALL *CmdDrawMeshTasksIndirect)    (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(Buffer) buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, OpenRhiOptional  const OpenRhiPtr(Buffer) countBuffer, uint64_t countBufferOffset); // buffer contains "DrawMeshTasksDesc" commands
    // }
};

OpenRhiNamespaceEnd
