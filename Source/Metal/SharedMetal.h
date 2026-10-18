// © 2026 NVIDIA Corporation

#pragma once

// Keep metal-cpp's private implementation symbols out of the OpenRHI binary's exported symbol table
#define METALCPP_SYMBOL_VISIBILITY_HIDDEN

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <mutex>
#include <shared_mutex>

#include "SharedExternal.h"

namespace openrhi {

struct DeviceMetal;
struct MemoryMetal;
struct BufferMetal;
struct AccelerationStructureMetal;
struct TextureMetal;
struct QueueMetal;
struct FenceMetal;
struct CommandAllocatorMetal;
struct CommandBufferMetal;
struct DescriptorMetal;
struct DescriptorPoolMetal;
struct DescriptorSetMetal;
struct PipelineLayoutMetal;
struct PipelineMetal;
struct PipelineCacheMetal;
struct QueryPoolMetal;
struct SwapChainMetal;

// Metal factory methods and encoder creation return autoreleased objects. OpenRHI can be called from threads without
// an autorelease pool, so drain them locally on paths that run repeatedly (retain what must outlive the scope)
struct AutoreleasePoolMetal final {
    inline AutoreleasePoolMetal()
        : m_Pool(NS::AutoreleasePool::alloc()->init()) {
    }

    inline ~AutoreleasePoolMetal() {
        m_Pool->release();
    }

private:
    NS::AutoreleasePool* m_Pool;
};

// Metal command queues are untyped, so each supported "QueueType" can be backed by several native queues
constexpr uint32_t QUEUE_TYPE_NUM = 3;
constexpr uint32_t QUEUE_NUM_PER_TYPE = 4;
constexpr uint32_t DRAW_INDIRECT_MAX_NUM = 1 << 16;

// Native Slang argument-table slots (see "OpenRHI.slang")
constexpr uint32_t ARGUMENT_SLOT_RESOURCE_HEAP = 0;
constexpr uint32_t ARGUMENT_SLOT_SAMPLER_HEAP = 1;
constexpr uint32_t ARGUMENT_SLOT_ROOT = 2;
constexpr uint32_t ARGUMENT_SLOT_MULTIVIEW = 3; // native only
constexpr uint32_t ARGUMENT_SLOT_VERTEX_BUFFER_BASE = 6;

// Argument table slots of internal shaders ("Shaders/InternalMetal.slang"). "CmdClearAttachments" uses "INTERNAL_SLOT_CONSTANTS" in the
// app's argument table, it must not overlap resource heap, sampler heap and root slots. It shares a slot with "ARGUMENT_SLOT_MULTIVIEW",
// which is rebound by the next draw (the pipeline becomes dirty)
constexpr uint32_t INTERNAL_SLOT_KERNEL_CONSTANTS = 0; // indirect draw preparation
constexpr uint32_t INTERNAL_SLOT_CONSTANTS = 3;
constexpr uint32_t INTERNAL_SLOT_CLEAR_VALUE = 4;
constexpr uint32_t INTERNAL_SLOT_CLEAR_OFFSET = 5; // 3D slice or typed buffer element offset
constexpr uint32_t INTERNAL_SLOT_TEXTURE = 0;

static_assert(INTERNAL_SLOT_CONSTANTS > ARGUMENT_SLOT_ROOT, "'INTERNAL_SLOT_CONSTANTS' must not overlap heap and root slots");

// Draw emulation register space
constexpr uint32_t DRAW_EMULATION_SPACE = 999; // "OPENRHI_BASE_ATTRIBUTES_EMULATION_SPACE" in "OpenRHI.slang"

// Descriptor table entry ("OpenRhiDescriptorEntry")
struct DescriptorEntryMetal {
    uint64_t bufferAddress; // buffer GPU address or sampler resource ID
    uint64_t resourceId;    // texture resource ID
    uint64_t metadata;      // see "OpenRHI.slang"
};

static_assert(sizeof(DescriptorEntryMetal) == 24, "Native descriptor ABI changed");

constexpr uint64_t DESCRIPTOR_ENTRY_SIZE = sizeof(DescriptorEntryMetal);

// Texture atomics: 32-bit "R32Uint" and "R32Sint", 64-bit min/max "RG32Uint" (Apple8+)
static inline bool IsAtomicFormat(MTL::Device& device, Format format) {
    if (format == Format::RG32_UINT)
        return device.supportsFamily(MTL::GPUFamilyApple8);

    return format == Format::R32_UINT || format == Format::R32_SINT;
}

// Metal 4 function descriptor for a library function, optionally specialized with function constants and / or renamed
// (a unique name is required for visible functions of different libraries linked into one pipeline). Owned by the caller
static inline MTL4::FunctionDescriptor* NewFunctionDescriptor(MTL::Library* library, const char* name, const MTL::FunctionConstantValues* constants = nullptr, const char* specializedName = nullptr) {
    MTL4::LibraryFunctionDescriptor* function = MTL4::LibraryFunctionDescriptor::alloc()->init();
    function->setLibrary(library);
    function->setName(NS::String::string(name, NS::UTF8StringEncoding));

    if (!constants && !specializedName)
        return function;

    MTL4::SpecializedFunctionDescriptor* specialized = MTL4::SpecializedFunctionDescriptor::alloc()->init();
    specialized->setFunctionDescriptor(function);
    function->release();

    if (constants)
        specialized->setConstantValues(constants);

    if (specializedName)
        specialized->setSpecializedName(NS::String::string(specializedName, NS::UTF8StringEncoding));

    return specialized;
}

} // namespace openrhi

#include "DeviceMetal.h"
