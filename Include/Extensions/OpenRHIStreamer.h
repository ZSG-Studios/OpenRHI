// © 2024 NVIDIA Corporation

// Goal: data streaming

#pragma once

#define OPENRHI_STREAMER_H 1

/*
Single-threaded usage:
- stream constants, host data or dynamic-buffer data as needed
- for destination copies, begin a batch, set it in each "StreamBufferDataDesc" or "StreamTextureDataDesc" and record "CmdCopyStreamedData" once
- call "EndStreamerFrame" after all command recording that uses streamed data is complete

Multi-threaded usage:
- one "Streamer" can serve multiple threads if "OPENRHI_STREAMER_THREAD_SAFE" is enabled; otherwise synchronize all access externally
- use a different copy batch for each independently populated command buffer and finish populating it before "CmdCopyStreamedData"
- synchronize all users before "EndStreamerFrame" and size the constant and host-data rings to avoid overwriting live data
*/

OpenRhiNamespaceBegin

OpenRhiForwardStruct(Streamer);

// Identifier for a batch of destination copy requests scoped to a "Streamer" (0 is invalid)
typedef uint64_t OpenRhi(StreamerCopyBatch);

OpenRhiStruct(StreamerDesc) {
    // Statically allocated ring-buffer for dynamic constants
    OpenRhiOptional OpenRhi(MemoryLocation) constantBufferMemoryLocation;   // UPLOAD or DEVICE_UPLOAD
    OpenRhiOptional uint64_t constantBufferSize;                        // should be large enough to avoid overwriting data for enqueued frames

    // Dynamically (re)allocated ring-buffer for copying and rendering
    OpenRhi(MemoryLocation) dynamicBufferMemoryLocation;                // UPLOAD or DEVICE_UPLOAD
    OpenRhi(BufferDesc) dynamicBufferDesc;                              // "size" is ignored
    uint32_t queuedFrameNum;                                        // number of frames "in-flight" (usually 1-3), adds 1 under the hood for the current "not-yet-committed" frame

    // Statically allocated read/write ring-buffer for generic CPU data
    OpenRhiOptional uint64_t hostDataCapacity;                          // capacity for all simultaneously live data and alignment padding
};

OpenRhiStruct(StreamBufferDataDesc) {
    // Data to upload
    const OpenRhiPtr(DataSize) dataChunks;              // will be concatenated in dynamic buffer memory
    uint32_t dataChunkNum;
    uint32_t placementAlignment;                    // desired alignment for "BufferOffset::offset"

    // Destination
    OpenRhiOptional OpenRhi(StreamerCopyBatch) copyBatch;   // required if "dstBuffer" is not NULL
    OpenRhiOptional OpenRhiPtr(Buffer) dstBuffer;
    OpenRhiOptional uint64_t dstOffset;
};

OpenRhiStruct(StreamTextureDataDesc) {
    // Data to upload
    const void* data;
    uint32_t dataRowPitch;
    uint32_t dataSlicePitch;

    // Destination
    OpenRhi(StreamerCopyBatch) copyBatch;
    OpenRhiPtr(Texture) dstTexture;
    OpenRhiOptional OpenRhi(TextureRegionDesc) dstRegion;
};

// Threadsafe: yes by default (see OPENRHI_STREAMER_THREAD_SAFE CMake option)
// Different batches can be populated concurrently; using the same batch concurrently requires external synchronization
OpenRhiStruct(StreamerInterface) {
    OpenRhi(Result)             (OPENRHI_CALL *CreateStreamer)              (OpenRhiRef(Device) device, const OpenRhiRef(StreamerDesc) streamerDesc, OpenRhiOut OpenRhiRef(Streamer*) streamer);
    void                    (OPENRHI_CALL *DestroyStreamer)             (OpenRhiPtr(Streamer) streamer);

    // Statically allocated (never changes)
    OpenRhiPtr(Buffer)          (OPENRHI_CALL *GetStreamerConstantBuffer)   (OpenRhiRef(Streamer) streamer);

    // (HOST) Streams data to a constant buffer. Returns "offset" in "GetStreamerConstantBuffer" for direct usage in the current frame
    uint32_t                (OPENRHI_CALL *StreamConstantData)          (OpenRhiRef(Streamer) streamer, const void* data, uint32_t dataSize);

    // (HOST) Streams generic CPU data. Returns a read/write pointer that remains valid until the ring buffer wraps. "placementAlignment" must be a power of 2 (0 is treated as 1)
    void*                   (OPENRHI_CALL *StreamHostData)              (OpenRhiRef(Streamer) streamer, const void* data, uint64_t dataSize, uint32_t placementAlignment);

    // Copy batch identifier (unconsumed batches are discarded by "EndStreamerFrame")
    OpenRhi(StreamerCopyBatch)  (OPENRHI_CALL *BeginStreamerCopyBatch)      (OpenRhiRef(Streamer) streamer);

    // (HOST) Stream data to a dynamic buffer. Return "buffer & offset" for direct usage in the current frame
    OpenRhi(BufferOffset)       (OPENRHI_CALL *StreamBufferData)            (OpenRhiRef(Streamer) streamer, const OpenRhiRef(StreamBufferDataDesc) streamBufferDataDesc);
    OpenRhi(BufferOffset)       (OPENRHI_CALL *StreamTextureData)           (OpenRhiRef(Streamer) streamer, const OpenRhiRef(StreamTextureDataDesc) streamTextureDataDesc);

    // Command buffer
    // {
        // (DEVICE) Copies batched data to destinations, which must be in "COPY_DESTINATION" state. Consumes "copyBatch", making it invalid
        void                (OPENRHI_CALL *CmdCopyStreamedData)         (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(Streamer) streamer, OpenRhi(StreamerCopyBatch) copyBatch);
    // }

    // (HOST) Must be called once at the very end of the frame (also discards unconsumed copy batches)
    void                    (OPENRHI_CALL *EndStreamerFrame)            (OpenRhiRef(Streamer) streamer);
};

OpenRhiNamespaceEnd
