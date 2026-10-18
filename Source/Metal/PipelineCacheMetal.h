// © 2026 NVIDIA Corporation

#pragma once

#include <limits.h>

namespace openrhi {

// A temporary file (the archive API requires a file URL)
struct ArchiveFileMetal {
    char path[PATH_MAX];
};

// A pipeline compiled on a miss, captured in "GetData"
struct PendingPipelineMetal {
    MTL4::PipelineDescriptor* render;
    MTL4::ComputePipelineDescriptor* compute;
    MTL4::PipelineStageDynamicLinkingDescriptor* linking;
};

// Pipelines are looked up in the initial data ("MTL4Archive"), misses are compiled by the device compiler and their descriptors are recorded.
// "GetData" captures recorded pipelines by the cache's compiler ("MTL4PipelineDataSetSerializer") and flushes them into a new archive,
// the data includes all loaded and flushed archives.
struct PipelineCacheMetal final : public DebugNameBase {
    PipelineCacheMetal(DeviceMetal& device);
    ~PipelineCacheMetal();
    Result Create(const PipelineCacheDesc& desc);
    Result GetData(void* dst, uint64_t& size) const;
    DeviceMetal& GetDevice() const;

    // "linking" is optional. The pipeline is owned by the caller, "nullptr" on failure (a miss if "failOnMiss")
    MTL::ComputePipelineState* NewComputePipeline(const MTL4::ComputePipelineDescriptor* desc, const MTL4::PipelineStageDynamicLinkingDescriptor* linking, bool failOnMiss, NS::Error** error);
    MTL::RenderPipelineState* NewRenderPipeline(const MTL4::PipelineDescriptor* desc, bool failOnMiss, NS::Error** error);

    void SetDebugName(const char* name) OPENRHI_DEBUG_NAME_OVERRIDE;

private:
    template <typename T, typename Lookup>
    T* FindPipeline(Lookup lookup) const;

    void AddPendingPipeline(const PendingPipelineMetal& pendingPipeline);
    bool CapturePendingPipelines() const;

    DeviceMetal& m_Device;
    MTL4::PipelineDataSetSerializer* m_Serializer = nullptr;
    MTL4::Compiler* m_Compiler = nullptr;
    Vector<MTL4::Archive*> m_Archives;                       // loaded initial data, immutable after "Create"
    mutable Vector<ArchiveFileMetal> m_ArchiveFiles;         // loaded and flushed archives
    mutable Vector<PendingPipelineMetal> m_PendingPipelines; // owned copies
    mutable Lock m_Lock;                                     // "GetData" ("m_ArchiveFiles", "m_Compiler", "m_Serializer", "m_IsCaptured")
    mutable Lock m_PendingLock;                              // "m_PendingPipelines"
    mutable bool m_IsCaptured = false;                       // the serializer holds pipelines captured since the last serialization
};

} // namespace openrhi
