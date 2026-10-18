// © 2026 NVIDIA Corporation

OPENRHI_INLINE Result DescriptorHeapVal::WriteResourceDescriptors(const WriteResourceDescriptorsDesc* writeDescs, uint32_t writeDescNum) {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, writeDescNum == 0 || writeDescs, Result::INVALID_ARGUMENT, "'writeDescs' is NULL");

    Scratch<WriteResourceDescriptorsDesc> writeDescsImpl = OPENRHI_ALLOCATE_SCRATCH(m_Device, WriteResourceDescriptorsDesc, writeDescNum);
    for (uint32_t i = 0; i < writeDescNum; i++) {
        const WriteResourceDescriptorsDesc& writeDesc = writeDescs[i];
        OPENRHI_RETURN_ON_FAILURE(&m_Device, writeDesc.resource, Result::INVALID_ARGUMENT, "'writeDescs[%u].resource' is NULL", i);

        const DescriptorVal& descriptor = *(DescriptorVal*)writeDesc.resource;
        OPENRHI_RETURN_ON_FAILURE(&m_Device, descriptor.GetType() < DescriptorType::MAX_NUM && descriptor.GetType() != DescriptorType::SAMPLER && descriptor.GetType() != DescriptorType::MUTABLE && descriptor.GetType() != DescriptorType::INPUT_ATTACHMENT, Result::INVALID_ARGUMENT, "'writeDescs[%u].resource' has an invalid descriptor type", i);
        OPENRHI_RETURN_ON_FAILURE(&m_Device, writeDesc.descriptorIndex < m_Desc.resourceDescriptorNum, Result::INVALID_ARGUMENT, "'writeDescs[%u].descriptorIndex' is out of bounds", i);

        writeDescsImpl[i] = writeDesc;
        writeDescsImpl[i].resource = descriptor.GetImpl();
    }

    if (writeDescNum > 1) {
        std::sort((WriteResourceDescriptorsDesc*)writeDescsImpl, (WriteResourceDescriptorsDesc*)writeDescsImpl + writeDescNum, [](const WriteResourceDescriptorsDesc& a, const WriteResourceDescriptorsDesc& b) { return a.descriptorIndex < b.descriptorIndex; });
        for (uint32_t i = 1; i < writeDescNum; i++)
            OPENRHI_RETURN_ON_FAILURE(&m_Device, writeDescsImpl[i - 1].descriptorIndex != writeDescsImpl[i].descriptorIndex, Result::INVALID_ARGUMENT, "'descriptorIndex=%u' is duplicated", writeDescsImpl[i].descriptorIndex);
    }

    return m_Device.GetDescriptorHeapInterfaceImpl().WriteResourceDescriptors(*GetImpl(), writeDescsImpl, writeDescNum);
}

OPENRHI_INLINE Result DescriptorHeapVal::WriteSamplerDescriptors(const WriteSamplerDescriptorsDesc* writeDescs, uint32_t writeDescNum) {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, writeDescNum == 0 || writeDescs, Result::INVALID_ARGUMENT, "'writeDescs' is NULL");

    Scratch<WriteSamplerDescriptorsDesc> writeDescsImpl = OPENRHI_ALLOCATE_SCRATCH(m_Device, WriteSamplerDescriptorsDesc, writeDescNum);
    for (uint32_t i = 0; i < writeDescNum; i++) {
        const WriteSamplerDescriptorsDesc& writeDesc = writeDescs[i];
        OPENRHI_RETURN_ON_FAILURE(&m_Device, writeDesc.sampler, Result::INVALID_ARGUMENT, "'writeDescs[%u].sampler' is NULL", i);

        const DescriptorVal& descriptor = *(DescriptorVal*)writeDesc.sampler;
        OPENRHI_RETURN_ON_FAILURE(&m_Device, descriptor.GetType() == DescriptorType::SAMPLER, Result::INVALID_ARGUMENT, "'writeDescs[%u].sampler' is not a sampler", i);
        OPENRHI_RETURN_ON_FAILURE(&m_Device, writeDesc.descriptorIndex < m_Desc.samplerDescriptorNum, Result::INVALID_ARGUMENT, "'writeDescs[%u].descriptorIndex' is out of bounds", i);

        writeDescsImpl[i] = writeDesc;
        writeDescsImpl[i].sampler = descriptor.GetImpl();
    }

    if (writeDescNum > 1) {
        std::sort((WriteSamplerDescriptorsDesc*)writeDescsImpl, (WriteSamplerDescriptorsDesc*)writeDescsImpl + writeDescNum, [](const WriteSamplerDescriptorsDesc& a, const WriteSamplerDescriptorsDesc& b) { return a.descriptorIndex < b.descriptorIndex; });
        for (uint32_t i = 1; i < writeDescNum; i++)
            OPENRHI_RETURN_ON_FAILURE(&m_Device, writeDescsImpl[i - 1].descriptorIndex != writeDescsImpl[i].descriptorIndex, Result::INVALID_ARGUMENT, "'descriptorIndex=%u' is duplicated", writeDescsImpl[i].descriptorIndex);
    }

    return m_Device.GetDescriptorHeapInterfaceImpl().WriteSamplerDescriptors(*GetImpl(), writeDescsImpl, writeDescNum);
}
