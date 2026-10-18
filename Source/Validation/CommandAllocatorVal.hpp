// © 2021 NVIDIA Corporation

OPENRHI_INLINE Result CommandAllocatorVal::CreateCommandBuffer(CommandBuffer*& commandBuffer) {
    CommandBuffer* commandBufferImpl;
    const Result result = GetCoreInterfaceImpl().CreateCommandBuffer(*GetImpl(), commandBufferImpl);

    commandBuffer = nullptr;
    if (result == Result::SUCCESS)
        commandBuffer = (CommandBuffer*)Allocate<CommandBufferVal>(m_Device.GetAllocationCallbacks(), m_Device, commandBufferImpl, m_QueueType, false);

    return result;
}

OPENRHI_INLINE void CommandAllocatorVal::Reset() {
    GetCoreInterfaceImpl().ResetCommandAllocator(*GetImpl());
}
