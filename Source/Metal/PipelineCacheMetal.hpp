// © 2026 NVIDIA Corporation

#include <sys/stat.h>
#include <unistd.h>

// "newArchiveWithURL" can crash on malformed data, thus data without a valid header is rejected as "OUT_OF_DATE".
// Layout: header, archives ("uint64_t" size followed by the archive)
struct PipelineCacheHeaderMetal {
    uint32_t magic;
    uint32_t version;
    uint64_t archiveSize; // all archives
    uint64_t hash;        // of the following data
};

constexpr uint32_t PIPELINE_CACHE_MAGIC = 0x4D34524F; // "OR4M"
constexpr uint32_t PIPELINE_CACHE_VERSION = 4;
constexpr char TEMP_FILE_NAME[] = "openrhi-cache-XXXXXX";

// Reserves a unique file in the user's temporary directory
static inline bool CreateTempFile(char (&path)[PATH_MAX], const void* data, size_t size) {
    const size_t length = confstr(_CS_DARWIN_USER_TEMP_DIR, path, sizeof(path));

    if (!length || (length + sizeof(TEMP_FILE_NAME)) > sizeof(path)) {
        path[0] = 0;

        return false;
    }

    strcat(path, TEMP_FILE_NAME);
    const int fd = mkstemp(path);

    if (fd == -1) {
        path[0] = 0;

        return false;
    }

    FILE* file = fdopen(fd, "wb");

    if (!file) {
        close(fd);

        return false;
    }

    const bool written = !size || fwrite(data, 1, size, file) == size;

    return !fclose(file) && written;
}

static inline NS::URL* GetFileUrl(const char* path) {
    return NS::URL::fileURLWithPath(NS::String::string(path, NS::UTF8StringEncoding));
}

static inline void ReleasePendingPipeline(const PendingPipelineMetal& pendingPipeline) {
    if (pendingPipeline.render)
        pendingPipeline.render->release();

    if (pendingPipeline.compute)
        pendingPipeline.compute->release();

    if (pendingPipeline.linking)
        pendingPipeline.linking->release();
}

PipelineCacheMetal::PipelineCacheMetal(DeviceMetal& device)
    : m_Device(device), m_Archives(device.GetStdAllocator()), m_ArchiveFiles(device.GetStdAllocator()), m_PendingPipelines(device.GetStdAllocator()) {
}

PipelineCacheMetal::~PipelineCacheMetal() {
    for (const PendingPipelineMetal& pendingPipeline : m_PendingPipelines)
        ReleasePendingPipeline(pendingPipeline);

    for (MTL4::Archive* archive : m_Archives)
        archive->release();

    if (m_Compiler)
        m_Compiler->release();

    if (m_Serializer)
        m_Serializer->release();

    for (const ArchiveFileMetal& file : m_ArchiveFiles)
        unlink(file.path);
}

Result PipelineCacheMetal::Create(const PipelineCacheDesc& desc) {
    AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

    MTL4::PipelineDataSetSerializerDescriptor* serializerDesc = MTL4::PipelineDataSetSerializerDescriptor::alloc()->init();
    serializerDesc->setConfiguration(MTL4::PipelineDataSetSerializerConfigurationCaptureBinaries);
    m_Serializer = m_Device.GetNativeObject()->newPipelineDataSetSerializer(serializerDesc);
    serializerDesc->release();

    if (!m_Serializer)
        return Result::FAILURE;

    MTL4::CompilerDescriptor* compilerDesc = MTL4::CompilerDescriptor::alloc()->init();
    compilerDesc->setPipelineDataSetSerializer(m_Serializer);

    NS::Error* error = nullptr;
    m_Compiler = m_Device.GetNativeObject()->newCompiler(compilerDesc, &error);
    compilerDesc->release();

    if (!m_Compiler) {
        OPENRHI_REPORT_ERROR(&m_Device, "newCompiler() failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

        return Result::FAILURE;
    }

    if (!desc.data || !desc.size)
        return Result::SUCCESS;

    PipelineCacheHeaderMetal header = {};

    if (desc.size < sizeof(header))
        return Result::OUT_OF_DATE;

    memcpy(&header, desc.data, sizeof(header));

    const uint8_t* archives = (const uint8_t*)desc.data + sizeof(header);
    const size_t dataSize = desc.size - sizeof(header);

    if (header.magic != PIPELINE_CACHE_MAGIC || header.version != PIPELINE_CACHE_VERSION || header.archiveSize != dataSize || header.hash != Fnv1a64(FNV_INIT, archives, dataSize))
        return Result::OUT_OF_DATE;

    for (size_t offset = 0; offset < header.archiveSize;) {
        uint64_t archiveSize = 0;

        if (header.archiveSize - offset < sizeof(archiveSize))
            return Result::OUT_OF_DATE;

        memcpy(&archiveSize, archives + offset, sizeof(archiveSize));
        offset += sizeof(archiveSize);

        if (!archiveSize || archiveSize > (header.archiveSize - offset))
            return Result::OUT_OF_DATE;

        offset += archiveSize;
    }

    for (size_t offset = 0; offset < header.archiveSize;) {
        uint64_t archiveSize = 0;
        memcpy(&archiveSize, archives + offset, sizeof(archiveSize));

        const uint8_t* archiveData = archives + offset + sizeof(archiveSize);
        offset += sizeof(archiveSize) + archiveSize;

        ArchiveFileMetal file = {};

        if (!CreateTempFile(file.path, archiveData, archiveSize)) {
            if (file.path[0])
                unlink(file.path);

            return Result::FAILURE;
        }

        // Incompatible archives (i.e. from another OS version) are reported via "error" and dropped
        MTL4::Archive* archive = m_Device.GetNativeObject()->newArchive(GetFileUrl(file.path), &error);

        if (!archive) {
            OPENRHI_REPORT_WARNING(&m_Device, "Pipeline cache archive is out of date: %s", error ? error->localizedDescription()->utf8String() : "unknown error");
            unlink(file.path);

            continue;
        }

        m_Archives.push_back(archive);
        m_ArchiveFiles.push_back(file);
    }

    return Result::SUCCESS;
}

template <typename T, typename Lookup>
T* PipelineCacheMetal::FindPipeline(Lookup lookup) const {
    for (MTL4::Archive* archive : m_Archives) {
        NS::Error* lookupError = nullptr;
        T* pipeline = lookup(archive, &lookupError);

        if (pipeline)
            return pipeline;
    }

    return nullptr;
}

void PipelineCacheMetal::AddPendingPipeline(const PendingPipelineMetal& pendingPipeline) {
    ExclusiveScope lock(m_PendingLock);
    m_PendingPipelines.push_back(pendingPipeline);
}

// A capturing compiler doesn't populate the system shader cache, but uses it. Thus misses are compiled once by the device compiler, and captures in "GetData"
// are usually system shader cache hits. "CaptureDescriptors" is not enough, since "serializeAsArchiveAndFlushToURL" requires "CaptureBinaries"
bool PipelineCacheMetal::CapturePendingPipelines() const {
    Vector<PendingPipelineMetal> pendingPipelines(m_Device.GetStdAllocator());
    {
        ExclusiveScope lock(m_PendingLock);
        pendingPipelines.swap(m_PendingPipelines);
    }

    for (const PendingPipelineMetal& pendingPipeline : pendingPipelines) {
        NS::Error* error = nullptr;
        NS::Object* pipeline = nullptr;

        if (pendingPipeline.render)
            pipeline = m_Compiler->newRenderPipelineState(pendingPipeline.render, nullptr, &error);
        else if (pendingPipeline.linking)
            pipeline = m_Compiler->newComputePipelineState(pendingPipeline.compute, pendingPipeline.linking, nullptr, &error);
        else
            pipeline = m_Compiler->newComputePipelineState(pendingPipeline.compute, nullptr, &error);

        if (pipeline) {
            pipeline->release();
            m_IsCaptured = true;
        } else // the pipeline is not cached
            OPENRHI_REPORT_WARNING(&m_Device, "Pipeline cache update failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

        ReleasePendingPipeline(pendingPipeline);
    }

    return m_IsCaptured;
}

MTL::ComputePipelineState* PipelineCacheMetal::NewComputePipeline(const MTL4::ComputePipelineDescriptor* desc, const MTL4::PipelineStageDynamicLinkingDescriptor* linking, bool failOnMiss, NS::Error** error) {
    MTL::ComputePipelineState* pipeline = FindPipeline<MTL::ComputePipelineState>([&](MTL4::Archive* archive, NS::Error** lookupError) {
        return linking ? archive->newComputePipelineState(desc, linking, lookupError) : archive->newComputePipelineState(desc, lookupError);
    });

    if (pipeline || failOnMiss)
        return pipeline;

    MTL4::Compiler* compiler = m_Device.GetCompiler();
    pipeline = linking ? compiler->newComputePipelineState(desc, linking, nullptr, error) : compiler->newComputePipelineState(desc, nullptr, error);

    if (pipeline)
        AddPendingPipeline({nullptr, desc->copy(), linking ? linking->copy() : nullptr});

    return pipeline;
}

MTL::RenderPipelineState* PipelineCacheMetal::NewRenderPipeline(const MTL4::PipelineDescriptor* desc, bool failOnMiss, NS::Error** error) {
    MTL::RenderPipelineState* pipeline = FindPipeline<MTL::RenderPipelineState>([&](MTL4::Archive* archive, NS::Error** lookupError) {
        return archive->newRenderPipelineState(desc, lookupError);
    });

    if (pipeline || failOnMiss)
        return pipeline;

    pipeline = m_Device.GetCompiler()->newRenderPipelineState(desc, nullptr, error);

    if (pipeline)
        AddPendingPipeline({desc->copy(), nullptr, nullptr});

    return pipeline;
}

OPENRHI_INLINE Result PipelineCacheMetal::GetData(void* dst, uint64_t& size) const {
    ExclusiveScope lock(m_Lock);

    // "Flush" clears captured pipelines, thus each serialization is a new archive. The serializer refuses to serialize if nothing has been captured
    {
        AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

        if (CapturePendingPipelines()) {
            ArchiveFileMetal file = {};
            NS::Error* error = nullptr;

            if (!CreateTempFile(file.path, nullptr, 0) || !m_Serializer->serializeAsArchiveAndFlushToURL(GetFileUrl(file.path), &error)) {
                if (file.path[0])
                    unlink(file.path);

                OPENRHI_REPORT_ERROR(&m_Device, "Pipeline cache serialization failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

                return Result::FAILURE;
            }

            m_ArchiveFiles.push_back(file);
            m_IsCaptured = false;
        }
    }

    uint64_t archiveSize = 0;

    for (const ArchiveFileMetal& file : m_ArchiveFiles) {
        struct stat fileStat = {};

        if (stat(file.path, &fileStat))
            return Result::FAILURE;

        archiveSize += sizeof(uint64_t) + (uint64_t)fileStat.st_size;
    }

    uint8_t* archives = dst ? (uint8_t*)dst + sizeof(PipelineCacheHeaderMetal) : nullptr;
    const uint64_t dataSize = archiveSize ? sizeof(PipelineCacheHeaderMetal) + archiveSize : 0;

    size_t offset = 0;

    for (size_t i = 0; archives && dataSize && size >= dataSize && i < m_ArchiveFiles.size(); i++) {
        FILE* file = fopen(m_ArchiveFiles[i].path, "rb");

        if (!file)
            return Result::FAILURE;

        const bool isRead = !fseek(file, 0, SEEK_END) && ftell(file) >= 0;
        const uint64_t fileSize = isRead ? (uint64_t)ftell(file) : 0;
        rewind(file);

        const bool isCopied = isRead && (offset + sizeof(fileSize) + fileSize) <= archiveSize && fread(archives + offset + sizeof(fileSize), 1, fileSize, file) == fileSize;
        fclose(file);

        if (!isCopied)
            return Result::FAILURE;

        memcpy(archives + offset, &fileSize, sizeof(fileSize));
        offset += sizeof(fileSize) + fileSize;
    }

    Result result = Result::SUCCESS;

    if (archives && dataSize) {
        if (size < dataSize)
            result = Result::OUT_OF_MEMORY;
        else if (offset != archiveSize)
            result = Result::FAILURE;
        else {
            PipelineCacheHeaderMetal header = {};
            header.magic = PIPELINE_CACHE_MAGIC;
            header.version = PIPELINE_CACHE_VERSION;
            header.archiveSize = archiveSize;
            header.hash = Fnv1a64(FNV_INIT, archives, (size_t)(dataSize - sizeof(header)));

            memcpy(dst, &header, sizeof(header));
        }
    }

    size = dataSize;

    return result;
}

DeviceMetal& PipelineCacheMetal::GetDevice() const {
    return m_Device;
}

OPENRHI_INLINE void PipelineCacheMetal::SetDebugName(const char* name) {
    // The compiler label is immutable, archive ones aren't
    NS::String* label = NS::String::alloc()->init(name, NS::UTF8StringEncoding);

    for (MTL4::Archive* archive : m_Archives)
        archive->setLabel(label);

    label->release();
}
