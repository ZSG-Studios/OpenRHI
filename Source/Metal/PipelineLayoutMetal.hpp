// © 2026 NVIDIA Corporation

PipelineLayoutMetal::PipelineLayoutMetal(DeviceMetal& device)
    : m_Device(device)
    , m_Sets(device.GetStdAllocator())
    , m_ConstantOffsets(device.GetStdAllocator())
    , m_DescriptorOffsets(device.GetStdAllocator())
    , m_SetOffsets(device.GetStdAllocator())
    , m_RootSamplers(device.GetStdAllocator()) {
}

PipelineLayoutMetal::~PipelineLayoutMetal() {
    if (m_RootSamplerBuffer) {
        m_Device.RemoveResidency(m_RootSamplerBuffer);
        m_RootSamplerBuffer->release();
    }

    for (auto* sampler : m_RootSamplers)
        Destroy(sampler);
}

Result PipelineLayoutMetal::Create(const PipelineLayoutDesc& desc) {
    uint32_t offset = 0;

    // Draw emulation constants precede application root constants in the native root layout
    // They exist only if the corresponding "PipelineLayoutBits" are requested, regardless of the build configuration
    const bool vertexStage = (desc.shaderStages & StageBits::VERTEX_SHADER) != 0;

    if ((desc.flags & PipelineLayoutBits::ENABLE_DRAW_PARAMETERS_EMULATION) && vertexStage) {
        m_DrawParametersOffset = offset;
        offset += 2 * sizeof(uint32_t);
    }

    if ((desc.flags & PipelineLayoutBits::ENABLE_DRAW_INDEX_EMULATION) && vertexStage) {
        m_DrawIndexOffset = offset;
        offset += sizeof(uint32_t);
    }

    for (uint32_t i = 0; i < desc.rootConstantNum; i++) {
        m_ConstantOffsets.push_back(offset);
        offset += desc.rootConstants[i].size;
    }

    offset = Align(offset, (uint32_t)sizeof(uint64_t));

    for (uint32_t i = 0; i < desc.rootDescriptorNum; i++) {
        m_DescriptorOffsets.push_back(offset);
        offset += sizeof(uint64_t);
    }

    for (uint32_t i = 0; i < desc.descriptorSetNum; i++) {
        m_Sets.emplace_back(m_Device.GetStdAllocator());
        DescriptorSetMappingMetal& mapping = m_Sets.back();
        const DescriptorSetDesc& set = desc.descriptorSets[i];

        for (uint32_t j = 0; j < set.rangeNum; j++) {
            const DescriptorRangeDesc& range = set.ranges[j];

            DescriptorRangeMappingMetal m = {};
            m.sampler = range.descriptorType == DescriptorType::SAMPLER;
            m.offset = m.sampler ? mapping.samplerNum : mapping.resourceNum;
            m.descriptorNum = range.descriptorNum;

            if (m.sampler)
                mapping.samplerNum += range.descriptorNum;
            else
                mapping.resourceNum += range.descriptorNum;

            mapping.ranges.push_back(m);

            if (range.flags & DescriptorRangeBits::VARIABLE_SIZED_ARRAY)
                mapping.variableRange = j;
        }

        // The variable-sized range is moved to the end of its heap group
        if (mapping.variableRange != UINT32_MAX) {
            DescriptorRangeMappingMetal& variable = mapping.ranges[mapping.variableRange];

            for (uint32_t j = mapping.variableRange + 1; j < set.rangeNum; j++) {
                if (mapping.ranges[j].sampler == variable.sampler)
                    mapping.ranges[j].offset -= variable.descriptorNum;
            }

            variable.offset = (variable.sampler ? mapping.samplerNum : mapping.resourceNum) - variable.descriptorNum;
        }

        // D3D root tables separate resources from samplers. Give each nonempty group one pointer, resources first
        for (uint32_t sampler = 0; sampler < 2; sampler++) {
            bool present = false;

            for (uint32_t j = 0; j < set.rangeNum; j++) {
                const DescriptorRangeDesc& source = set.ranges[j];
                const DescriptorRangeMappingMetal& mappingRange = mapping.ranges[j];

                // "MUTABLE" ranges are directly indexed heaps. Input attachments are framebuffer fetches in a reserved space.
                // Neither uses root-table storage, but both keep their descriptor slots to preserve subsequent range offsets
                if (mappingRange.sampler != bool(sampler) || source.descriptorType == DescriptorType::MUTABLE || source.descriptorType == DescriptorType::INPUT_ATTACHMENT)
                    continue;

                present = true;
            }

            m_SetOffsets.push_back(present ? offset : UINT32_MAX);

            if (present) {
                offset += sizeof(uint64_t);
            }
        }
    }

    if (desc.rootSamplerNum) {
        m_RootSamplerOffset = offset;
        offset += sizeof(uint64_t);

        m_RootSamplerBuffer = m_Device.GetNativeObject()->newBuffer(desc.rootSamplerNum * DESCRIPTOR_ENTRY_SIZE, MTL::ResourceStorageModeShared);

        if (!m_RootSamplerBuffer)
            return Result::OUT_OF_MEMORY;

        m_Device.AddResidency(m_RootSamplerBuffer);

        for (uint32_t i = 0; i < desc.rootSamplerNum; i++) {
            DescriptorMetal* sampler = nullptr;
            Result result = m_Device.CreateImplementation<DescriptorMetal>(sampler, desc.rootSamplers[i].desc);

            if (result != Result::SUCCESS)
                return result;

            m_RootSamplers.push_back(sampler);
            sampler->WriteEntry((uint8_t*)m_RootSamplerBuffer->contents() + i * DESCRIPTOR_ENTRY_SIZE);
        }
    }

    m_RootDataSize = Align(offset, 16u);

    return Result::SUCCESS;
}

DeviceMetal& PipelineLayoutMetal::GetDevice() const {
    return m_Device;
}

uint32_t PipelineLayoutMetal::GetRootDataSize() const {
    return m_RootDataSize;
}

uint32_t PipelineLayoutMetal::GetRootConstantOffset(uint32_t index) const {
    return m_ConstantOffsets[index];
}

uint32_t PipelineLayoutMetal::GetRootDescriptorOffset(uint32_t index) const {
    return m_DescriptorOffsets[index];
}

uint32_t PipelineLayoutMetal::GetDrawParametersOffset() const {
    return m_DrawParametersOffset;
}

uint32_t PipelineLayoutMetal::GetDrawIndexOffset() const {
    return m_DrawIndexOffset;
}

bool PipelineLayoutMetal::IsDrawParametersEmulationEnabled() const {
    return m_DrawParametersOffset != UINT32_MAX;
}

bool PipelineLayoutMetal::IsDrawIndexEmulationEnabled() const {
    return m_DrawIndexOffset != UINT32_MAX;
}

const DescriptorSetMappingMetal& PipelineLayoutMetal::GetDescriptorSetMapping(uint32_t index) const {
    return m_Sets[index];
}

void PipelineLayoutMetal::InitRootData(void* data) const {
    if (m_RootDataSize)
        memset(data, 0, m_RootDataSize);

    if (m_RootSamplerBuffer) {
        uint64_t address = m_RootSamplerBuffer->gpuAddress();
        memcpy((uint8_t*)data + m_RootSamplerOffset, &address, sizeof(address));
    }
}

void PipelineLayoutMetal::WriteSetPointers(void* data, uint32_t index, const DescriptorSetMetal& set) const {
    const uint64_t addresses[] = {set.GetResourceAddress(), set.GetSamplerAddress()};

    for (uint32_t i = 0; i < 2; i++) {
        uint32_t offset = m_SetOffsets[index * 2 + i];

        if (offset != UINT32_MAX)
            memcpy((uint8_t*)data + offset, &addresses[i], sizeof(uint64_t));
    }
}
