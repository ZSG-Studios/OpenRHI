// © 2026 NVIDIA Corporation

static inline MTL::Size GetNativeThreadGroupSize(const ShaderDesc& shader) {
    // Validation requires non-zero sizes for native compute, mesh and task shaders
    return MTL::Size(std::max(shader.threadGroupSizeX, (Dim_t)1), std::max(shader.threadGroupSizeY, (Dim_t)1), std::max(shader.threadGroupSizeZ, (Dim_t)1));
}

constexpr std::array<MTL::BlendFactor, (size_t)BlendFactor::MAX_NUM> g_BlendFactors = {
    MTL::BlendFactorZero,                     // ZERO
    MTL::BlendFactorOne,                      // ONE
    MTL::BlendFactorSourceColor,              // SRC_COLOR
    MTL::BlendFactorOneMinusSourceColor,      // ONE_MINUS_SRC_COLOR
    MTL::BlendFactorDestinationColor,         // DST_COLOR
    MTL::BlendFactorOneMinusDestinationColor, // ONE_MINUS_DST_COLOR
    MTL::BlendFactorSourceAlpha,              // SRC_ALPHA
    MTL::BlendFactorOneMinusSourceAlpha,      // ONE_MINUS_SRC_ALPHA
    MTL::BlendFactorDestinationAlpha,         // DST_ALPHA
    MTL::BlendFactorOneMinusDestinationAlpha, // ONE_MINUS_DST_ALPHA
    MTL::BlendFactorBlendColor,               // CONSTANT_COLOR
    MTL::BlendFactorOneMinusBlendColor,       // ONE_MINUS_CONSTANT_COLOR
    MTL::BlendFactorBlendAlpha,               // CONSTANT_ALPHA
    MTL::BlendFactorOneMinusBlendAlpha,       // ONE_MINUS_CONSTANT_ALPHA
    MTL::BlendFactorSourceAlphaSaturated,     // SRC_ALPHA_SATURATE
    MTL::BlendFactorSource1Color,             // SRC1_COLOR
    MTL::BlendFactorOneMinusSource1Color,     // ONE_MINUS_SRC1_COLOR
    MTL::BlendFactorSource1Alpha,             // SRC1_ALPHA
    MTL::BlendFactorOneMinusSource1Alpha,     // ONE_MINUS_SRC1_ALPHA
};
OPENRHI_VALIDATE_ARRAY(g_BlendFactors);

constexpr std::array<MTL::BlendOperation, (size_t)BlendOp::MAX_NUM> g_BlendOps = {
    MTL::BlendOperationAdd,             // ADD
    MTL::BlendOperationSubtract,        // SUBTRACT
    MTL::BlendOperationReverseSubtract, // REVERSE_SUBTRACT
    MTL::BlendOperationMin,             // MIN
    MTL::BlendOperationMax,             // MAX
};
OPENRHI_VALIDATE_ARRAY(g_BlendOps);

constexpr std::array<MTL::StencilOperation, (size_t)StencilOp::MAX_NUM> g_StencilOps = {
    MTL::StencilOperationKeep,           // KEEP
    MTL::StencilOperationZero,           // ZERO
    MTL::StencilOperationReplace,        // REPLACE
    MTL::StencilOperationIncrementClamp, // INCREMENT_AND_CLAMP
    MTL::StencilOperationDecrementClamp, // DECREMENT_AND_CLAMP
    MTL::StencilOperationInvert,         // INVERT
    MTL::StencilOperationIncrementWrap,  // INCREMENT_AND_WRAP
    MTL::StencilOperationDecrementWrap,  // DECREMENT_AND_WRAP
};
OPENRHI_VALIDATE_ARRAY(g_StencilOps);

constexpr std::array<MTL::CullMode, (size_t)CullMode::MAX_NUM> g_CullModes = {
    MTL::CullModeNone,  // NONE
    MTL::CullModeFront, // FRONT
    MTL::CullModeBack,  // BACK
};
OPENRHI_VALIDATE_ARRAY(g_CullModes);

// Unsupported adjacency and patch topologies are rejected before native pipeline creation
constexpr std::array<MTL::PrimitiveType, (size_t)Topology::MAX_NUM> g_PrimitiveTypes = {
    MTL::PrimitiveTypePoint,         // POINT_LIST
    MTL::PrimitiveTypeLine,          // LINE_LIST
    MTL::PrimitiveTypeLineStrip,     // LINE_STRIP
    MTL::PrimitiveTypeTriangle,      // TRIANGLE_LIST
    MTL::PrimitiveTypeTriangleStrip, // TRIANGLE_STRIP
    MTL::PrimitiveTypeLine,          // LINE_LIST_WITH_ADJACENCY
    MTL::PrimitiveTypeLineStrip,     // LINE_STRIP_WITH_ADJACENCY
    MTL::PrimitiveTypeTriangle,      // TRIANGLE_LIST_WITH_ADJACENCY
    MTL::PrimitiveTypeTriangleStrip, // TRIANGLE_STRIP_WITH_ADJACENCY
    MTL::PrimitiveTypeTriangle,      // PATCH_LIST
};
OPENRHI_VALIDATE_ARRAY(g_PrimitiveTypes);

constexpr std::array<MTL::PrimitiveTopologyClass, (size_t)Topology::MAX_NUM> g_TopologyClasses = {
    MTL::PrimitiveTopologyClassPoint,    // POINT_LIST
    MTL::PrimitiveTopologyClassLine,     // LINE_LIST
    MTL::PrimitiveTopologyClassLine,     // LINE_STRIP
    MTL::PrimitiveTopologyClassTriangle, // TRIANGLE_LIST
    MTL::PrimitiveTopologyClassTriangle, // TRIANGLE_STRIP
    MTL::PrimitiveTopologyClassLine,     // LINE_LIST_WITH_ADJACENCY
    MTL::PrimitiveTopologyClassLine,     // LINE_STRIP_WITH_ADJACENCY
    MTL::PrimitiveTopologyClassTriangle, // TRIANGLE_LIST_WITH_ADJACENCY
    MTL::PrimitiveTopologyClassTriangle, // TRIANGLE_STRIP_WITH_ADJACENCY
    MTL::PrimitiveTopologyClassTriangle, // PATCH_LIST
};
OPENRHI_VALIDATE_ARRAY(g_TopologyClasses);

// A face with "CompareOp::NONE" keeps the default descriptor (always passes, keeps the value)
static inline void SetStencil(MTL::StencilDescriptor* dst, const StencilDesc& src) {
    if (src.compareOp == CompareOp::NONE)
        return;

    dst->setStencilCompareFunction(GetCompareMetal(src.compareOp));
    dst->setStencilFailureOperation(g_StencilOps[(uint32_t)src.failOp]);
    dst->setDepthStencilPassOperation(g_StencilOps[(uint32_t)src.passOp]);
    dst->setDepthFailureOperation(g_StencilOps[(uint32_t)src.depthFailOp]);
    dst->setReadMask(src.compareMask);
    dst->setWriteMask(src.writeMask);
}

static inline MTL::ComputePipelineState* NewComputePipeline(DeviceMetal& device, const PipelineCache* cache, bool failOnMiss, const MTL4::ComputePipelineDescriptor* desc, const MTL4::PipelineStageDynamicLinkingDescriptor* linking, NS::Error** error) {
    if (cache)
        return ((PipelineCacheMetal*)cache)->NewComputePipeline(desc, linking, failOnMiss, error);

    if (failOnMiss)
        return nullptr;

    MTL4::Compiler* compiler = device.GetCompiler();

    return linking ? compiler->newComputePipelineState(desc, linking, nullptr, error) : compiler->newComputePipelineState(desc, nullptr, error);
}

static inline MTL::RenderPipelineState* NewRenderPipeline(DeviceMetal& device, const PipelineCache* cache, bool failOnMiss, const MTL4::PipelineDescriptor* desc, NS::Error** error) {
    if (cache)
        return ((PipelineCacheMetal*)cache)->NewRenderPipeline(desc, failOnMiss, error);

    if (failOnMiss)
        return nullptr;

    return device.GetCompiler()->newRenderPipelineState(desc, nullptr, error);
}

PipelineMetal::PipelineMetal(DeviceMetal& device)
    : m_Device(device), m_ShaderGroupIdentifiers(device.GetStdAllocator()) {
}

PipelineMetal::~PipelineMetal() {
    if (m_Render)
        m_Render->release();

    if (m_Compute)
        m_Compute->release();

    if (m_VisibleFunctionTable) {
        m_Device.RemoveResidency(m_VisibleFunctionTable);
        m_VisibleFunctionTable->release();
    }

    if (m_IntersectionFunctionTable) {
        m_Device.RemoveResidency(m_IntersectionFunctionTable);
        m_IntersectionFunctionTable->release();
    }

    if (m_DepthStencil)
        m_DepthStencil->release();
}

Result PipelineMetal::LoadFunction(const ShaderDesc& shader, MTL::Library*& library, MTL4::FunctionDescriptor*& function, const ShaderLoadDescMetal& load) {
    const char* functionName = shader.entryPointName ? shader.entryPointName : "main";
    const uint8_t* bytecode = (const uint8_t*)shader.bytecode;
    size_t bytecodeSize = shader.size;
    if (bytecodeSize < 4 || memcmp(bytecode, "MTLB", 4)) {
        OPENRHI_REPORT_ERROR(&m_Device, "Metal shaders require Slang-generated metallib bytecode");

        return Result::INVALID_ARGUMENT;
    }

    // Pipelines are built by "MTL4Compiler", but metallibs are still loaded by the device ("MTL4LibraryDescriptor" compiles source only)
    dispatch_data_t data = dispatch_data_create(bytecode, bytecodeSize, dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), DISPATCH_DATA_DESTRUCTOR_DEFAULT);

    NS::Error* error = nullptr;
    library = m_Device.GetNativeObject()->newLibrary(data, &error);
    dispatch_release(data);

    if (!library)
        OPENRHI_REPORT_ERROR(&m_Device, "Metal library creation failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

    // Slang fragment shaders opt into the sample mask ABI through function constant 65535
    bool unsupportedSampleMask = false;

    if (library) {
        if (shader.stage == StageBits::FRAGMENT_SHADER) {
            MTL::Function* reflected = library->newFunction(NS::String::string(functionName, NS::UTF8StringEncoding));

            if (reflected) {
                constexpr NS::UInteger sampleMaskConstantIndex = 65535;

                bool conforms = false;
                NS::Dictionary* dictionary = reflected->functionConstantsDictionary();
                auto* keys = dictionary->keyEnumerator();

                while (NS::Object* key = keys->nextObject()) {
                    auto* constant = (MTL::FunctionConstant*)dictionary->object(key);
                    conforms |= constant->index() == sampleMaskConstantIndex && constant->type() == MTL::DataTypeUInt;
                }
                reflected->release();

                if (conforms) {
                    const uint32_t nativeSampleMask = load.sampleMask == ALL ? UINT32_MAX : load.sampleMask;

                    MTL::FunctionConstantValues* constants = MTL::FunctionConstantValues::alloc()->init();
                    constants->setConstantValue(&nativeSampleMask, MTL::DataTypeUInt, sampleMaskConstantIndex);
                    function = NewFunctionDescriptor(library, functionName, constants);
                    constants->release();
                } else if (load.sampleMask != ALL)
                    unsupportedSampleMask = true;
                else
                    function = NewFunctionDescriptor(library, functionName);
            } else
                OPENRHI_REPORT_ERROR(&m_Device, "Metal entry point '%s' was not found", functionName);
        } else
            function = NewFunctionDescriptor(library, functionName);
    }

    if (!library)
        return Result::FAILURE;

    if (unsupportedSampleMask) {
        OPENRHI_REPORT_WARNING(&m_Device, "'sampleMask' requires the native fragment shader to use the 'OpenRhiPipelineSampleMask' function constant");

        return Result::UNSUPPORTED;
    }

    return function ? Result::SUCCESS : Result::FAILURE;
}

Result PipelineMetal::Create(const ComputePipelineDesc& desc) {
    AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

    m_Layout = (const PipelineLayoutMetal*)desc.pipelineLayout;

    const bool failOnCacheMiss = desc.flags & ComputePipelineBits::FAIL_ON_CACHE_MISS;

    ShaderLoadDescMetal loadDesc = {};

    MTL::Library* library = nullptr;
    MTL4::FunctionDescriptor* function = nullptr;
    Result result = LoadFunction(desc.shader, library, function, loadDesc);

    if (result == Result::SUCCESS) {
        m_ThreadGroup = GetNativeThreadGroupSize(desc.shader);

        // Dispatches always use the pipeline threadgroup size
        MTL4::ComputePipelineDescriptor* pipelineDesc = MTL4::ComputePipelineDescriptor::alloc()->init();
        pipelineDesc->setComputeFunctionDescriptor(function);
        pipelineDesc->setRequiredThreadsPerThreadgroup(m_ThreadGroup);

        NS::Error* error = nullptr;
        m_Compute = NewComputePipeline(m_Device, desc.cache, failOnCacheMiss, pipelineDesc, nullptr, &error);

        if (!m_Compute) {
            result = Result::FAILURE;

            if (!failOnCacheMiss)
                OPENRHI_REPORT_ERROR(&m_Device, "Metal compute pipeline creation failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");
        }

        pipelineDesc->release();
    }

    if (function)
        function->release();

    if (library)
        library->release();

    return result;
}

// Native Metal shaders support inline ray tracing

Result PipelineMetal::Create(const RayTracingPipelineDesc&) {
    return Result::UNSUPPORTED;
}

OPENRHI_INLINE Result PipelineMetal::WriteShaderGroupIdentifiers(uint32_t, uint32_t, uint32_t, void*) const {
    return Result::UNSUPPORTED;
}

Result PipelineMetal::Create(const GraphicsPipelineDesc& desc) {
    m_Layout = (const PipelineLayoutMetal*)desc.pipelineLayout;
    m_Multiview = desc.outputMerger.multiview;
    m_ViewMask = desc.outputMerger.viewMask;

    bool hasMesh = false;
    bool hasGeometry = false;
    bool hasTessellation = false;
    bool hasFragment = false;

    for (uint32_t i = 0; i < desc.shaderNum; i++) {
        const ShaderDesc& shader = desc.shaders[i];
        hasMesh |= shader.stage == StageBits::MESH_SHADER;
        hasGeometry |= shader.stage == StageBits::GEOMETRY_SHADER;
        hasTessellation |= shader.stage == StageBits::TESS_CONTROL_SHADER || shader.stage == StageBits::TESS_EVALUATION_SHADER;
        hasFragment |= shader.stage == StageBits::FRAGMENT_SHADER;
    }

    if (hasGeometry || hasTessellation) {
        OPENRHI_REPORT_WARNING(&m_Device, "Metal graphics pipelines require vertex or mesh stages");

        return Result::UNSUPPORTED;
    }

    // The sample mask is applied by the fragment shader
    if (!hasFragment && desc.multisample && desc.multisample->sampleMask != ALL) {
        OPENRHI_REPORT_WARNING(&m_Device, "'sampleMask' requires a fragment shader");

        return Result::UNSUPPORTED;
    }

    // Native Metal supports point, line and triangle primitives
    const Topology topology = desc.inputAssembly.topology;
    const bool isAdjacency = topology >= Topology::LINE_LIST_WITH_ADJACENCY && topology <= Topology::TRIANGLE_STRIP_WITH_ADJACENCY;

    if (!hasMesh) {
        if ((isAdjacency && !hasGeometry) || (topology == Topology::PATCH_LIST && !hasTessellation) || topology == Topology::TRIANGLE_STRIP_WITH_ADJACENCY) {
            OPENRHI_REPORT_WARNING(&m_Device, "Adjacency topologies require a geometry shader, 'PATCH_LIST' requires tessellation, 'TRIANGLE_STRIP_WITH_ADJACENCY' is unsupported");

            return Result::UNSUPPORTED;
        }
    }

    const bool isMesh = hasMesh;

    // Validate native vertex formats
    const uint32_t vertexAttributeNum = (desc.vertexInput && !hasMesh) ? desc.vertexInput->attributeNum : 0;

    for (uint32_t i = 0; i < vertexAttributeNum; i++) {
        const Format format = desc.vertexInput->attributes[i].format;

        if (GetVertexFormatMetal(format) == MTL::VertexFormatInvalid) {
            OPENRHI_REPORT_WARNING(&m_Device, "'vertexInput->attributes[%u].format' doesn't support 'FormatSupportBits::VERTEX_BUFFER'", i);

            return Result::UNSUPPORTED;
        }
    }

    const uint32_t attributeNum = (desc.vertexInput && !isMesh) ? desc.vertexInput->attributeNum : 0;
    AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

    MTL4::RenderPipelineDescriptor* pd = isMesh ? nullptr : MTL4::RenderPipelineDescriptor::alloc()->init();
    MTL4::MeshRenderPipelineDescriptor* mpd = isMesh ? MTL4::MeshRenderPipelineDescriptor::alloc()->init() : nullptr;
    // Shader functions remain alive through pipeline creation
    const uint32_t functionMaxNum = desc.shaderNum;
    Scratch<MTL::Library*> libraries = OPENRHI_ALLOCATE_SCRATCH(m_Device, MTL::Library*, desc.shaderNum);
    Scratch<MTL4::FunctionDescriptor*> functions = OPENRHI_ALLOCATE_SCRATCH(m_Device, MTL4::FunctionDescriptor*, functionMaxNum);
    uint32_t libraryNum = 0;
    uint32_t functionNum = 0;
    bool hasFragmentFunction = false;
    Result result = Result::SUCCESS;

    for (uint32_t i = 0; i < desc.shaderNum; i++) {
        const ShaderDesc& shader = desc.shaders[i];

        ShaderLoadDescMetal loadDesc = {};
        loadDesc.sampleMask = desc.multisample ? desc.multisample->sampleMask : ALL;
        m_MeshPayloadSize = std::max(m_MeshPayloadSize, shader.taskPayloadSize);

        MTL::Library* library = nullptr;
        MTL4::FunctionDescriptor* function = nullptr;
        result = LoadFunction(shader, library, function, loadDesc);

        if (result != Result::SUCCESS) {
            if (function)
                function->release();

            if (library)
                library->release();

            break;
        }

        libraries[libraryNum++] = library;
        functions[functionNum++] = function;

        if (shader.stage == StageBits::VERTEX_SHADER) {
            if (!isMesh)
                pd->setVertexFunctionDescriptor(function);
        } else if (shader.stage == StageBits::FRAGMENT_SHADER) {
            hasFragmentFunction = true;

            if (isMesh)
                mpd->setFragmentFunctionDescriptor(function);
            else
                pd->setFragmentFunctionDescriptor(function);
        } else if (shader.stage == StageBits::MESH_SHADER) {
            mpd->setMeshFunctionDescriptor(function);

            m_MeshGroup = GetNativeThreadGroupSize(shader);
        } else if (shader.stage == StageBits::TASK_SHADER) {
            mpd->setObjectFunctionDescriptor(function);

            m_TaskGroup = GetNativeThreadGroupSize(shader);
        }
    }

    // Vertex amplification IDs are dense: "[[amplification_id]]" indexes the set bits of the view mask.
    // Metal ignores view mappings if the max amplification count is 1, so a single sparse view still needs 2
    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    uint32_t amplificationCount = 1;

    if (m_Multiview == Multiview::VIEWPORT_BASED) // the pipeline "viewMask" is unused
        amplificationCount = deviceDesc.other.viewMaxNum;
    else if (m_ViewMask)
        amplificationCount = std::min(std::max((uint32_t)__builtin_popcount(m_ViewMask), 2u), (uint32_t)deviceDesc.other.viewMaxNum);

    const uint32_t sampleNum = desc.multisample ? desc.multisample->sampleNum : 1;
    const MTL4::AlphaToCoverageState alphaToCoverage = (desc.multisample && desc.multisample->alphaToCoverage) ? MTL4::AlphaToCoverageStateEnabled : MTL4::AlphaToCoverageStateDisabled;

    if (isMesh) {
        // Metal mesh pipelines require a fragment function even for depth-only draws
        if (!hasFragmentFunction)
            mpd->setFragmentFunctionDescriptor(m_Device.GetInternalShaders().GetDepthOnlyFragmentFunction());

        mpd->setMaxVertexAmplificationCount(amplificationCount);
        mpd->setRasterSampleCount(sampleNum);
        mpd->setAlphaToCoverageState(alphaToCoverage);

        mpd->setMaxTotalThreadsPerMeshThreadgroup(m_MeshGroup.width * m_MeshGroup.height * m_MeshGroup.depth);
        mpd->setRequiredThreadsPerMeshThreadgroup(m_MeshGroup);

        if (mpd->objectFunctionDescriptor()) {
            mpd->setMaxTotalThreadsPerObjectThreadgroup(m_TaskGroup.width * m_TaskGroup.height * m_TaskGroup.depth);
            mpd->setRequiredThreadsPerObjectThreadgroup(m_TaskGroup);
        }

        mpd->setPayloadMemoryLength(m_MeshPayloadSize);
    } else {
        pd->setInputPrimitiveTopology(g_TopologyClasses[(uint32_t)topology]);
        pd->setMaxVertexAmplificationCount(amplificationCount);
        pd->setRasterSampleCount(sampleNum);
        pd->setAlphaToCoverageState(alphaToCoverage);

        if (attributeNum) {
            MTL::VertexDescriptor* vertex = MTL::VertexDescriptor::alloc()->init();

            for (uint32_t i = 0; i < attributeNum; i++) {
                const VertexAttributeDesc& source = desc.vertexInput->attributes[i];

                MTL::VertexAttributeDescriptor* attribute = vertex->attributes()->object(i);
                attribute->setFormat(GetVertexFormatMetal(source.format));
                attribute->setOffset(source.offset);
                attribute->setBufferIndex(ARGUMENT_SLOT_VERTEX_BUFFER_BASE + desc.vertexInput->streams[source.streamIndex].bindingSlot);
            }

            for (uint32_t i = 0; i < desc.vertexInput->streamNum; i++) {
                const VertexStreamDesc& source = desc.vertexInput->streams[i];

                MTL::VertexBufferLayoutDescriptor* layout = vertex->layouts()->object(ARGUMENT_SLOT_VERTEX_BUFFER_BASE + source.bindingSlot);
                layout->setStride(MTL::BufferLayoutStrideDynamic);
                layout->setStepFunction(source.stepRate == VertexStreamStepRate::PER_INSTANCE ? MTL::VertexStepFunctionPerInstance : MTL::VertexStepFunctionPerVertex);
                layout->setStepRate(1);
            }

            pd->setVertexDescriptor(vertex);
            vertex->release();
        }
    }

    // Metal 4 pipelines don't include depth / stencil attachment formats
    MTL4::RenderPipelineColorAttachmentDescriptorArray* colorAttachments = isMesh ? mpd->colorAttachments() : pd->colorAttachments();

    for (uint32_t i = 0; i < desc.outputMerger.colorNum; i++) {
        const ColorAttachmentDesc& color = desc.outputMerger.colors[i];

        MTL::ColorWriteMask mask = MTL::ColorWriteMaskNone;

        if (color.colorWriteMask & ColorWriteBits::R)
            mask |= MTL::ColorWriteMaskRed;

        if (color.colorWriteMask & ColorWriteBits::G)
            mask |= MTL::ColorWriteMaskGreen;

        if (color.colorWriteMask & ColorWriteBits::B)
            mask |= MTL::ColorWriteMaskBlue;

        if (color.colorWriteMask & ColorWriteBits::A)
            mask |= MTL::ColorWriteMaskAlpha;

        MTL4::RenderPipelineColorAttachmentDescriptor* attachment = colorAttachments->object(i);
        attachment->setPixelFormat(GetPixelFormatMetal(color.format));
        attachment->setBlendingState(color.blendEnabled ? MTL4::BlendStateEnabled : MTL4::BlendStateDisabled);
        attachment->setWriteMask(mask);
        attachment->setSourceRGBBlendFactor(g_BlendFactors[(uint32_t)color.colorBlend.srcFactor]);
        attachment->setDestinationRGBBlendFactor(g_BlendFactors[(uint32_t)color.colorBlend.dstFactor]);
        attachment->setRgbBlendOperation(g_BlendOps[(uint32_t)color.colorBlend.op]);
        attachment->setSourceAlphaBlendFactor(g_BlendFactors[(uint32_t)color.alphaBlend.srcFactor]);
        attachment->setDestinationAlphaBlendFactor(g_BlendFactors[(uint32_t)color.alphaBlend.dstFactor]);
        attachment->setAlphaBlendOperation(g_BlendOps[(uint32_t)color.alphaBlend.op]);
    }

    if (result == Result::SUCCESS) {
        const bool failOnCacheMiss = desc.flags & GraphicsPipelineBits::FAIL_ON_CACHE_MISS;
        NS::Error* error = nullptr;

        if (result == Result::SUCCESS) {
            m_Render = NewRenderPipeline(m_Device, desc.cache, failOnCacheMiss, isMesh ? (MTL4::PipelineDescriptor*)mpd : (MTL4::PipelineDescriptor*)pd, &error);

            if (!m_Render) {
                result = Result::FAILURE;

                if (!failOnCacheMiss)
                    OPENRHI_REPORT_ERROR(&m_Device, "Metal render pipeline creation failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");
            }
        }
    }

    // "CompareOp::NONE" disables the test, including depth writes and stencil operations
    const DepthAttachmentDesc& depth = desc.outputMerger.depth;
    const StencilAttachmentDesc& stencil = desc.outputMerger.stencil;

    MTL::DepthStencilDescriptor* dd = MTL::DepthStencilDescriptor::alloc()->init();
    dd->setDepthCompareFunction(GetCompareMetal(depth.compareOp));
    dd->setDepthWriteEnabled(depth.compareOp != CompareOp::NONE && depth.write);

    if (stencil.front.compareOp != CompareOp::NONE || stencil.back.compareOp != CompareOp::NONE) {
        MTL::StencilDescriptor* front = MTL::StencilDescriptor::alloc()->init();
        SetStencil(front, stencil.front);

        MTL::StencilDescriptor* back = MTL::StencilDescriptor::alloc()->init();
        SetStencil(back, stencil.back);

        dd->setFrontFaceStencil(front);
        dd->setBackFaceStencil(back);

        front->release();
        back->release();
    }

    m_DepthStencil = m_Device.GetNativeObject()->newDepthStencilState(dd);
    dd->release();

    if (pd)
        pd->release();

    if (mpd)
        mpd->release();

    for (uint32_t i = 0; i < functionNum; i++)
        functions[i]->release();

    for (uint32_t i = 0; i < libraryNum; i++)
        libraries[i]->release();

    m_Primitive = g_PrimitiveTypes[(uint32_t)topology];
    m_Cull = g_CullModes[(uint32_t)desc.rasterization.cullMode];
    m_Winding = desc.rasterization.frontCounterClockwise ? MTL::WindingCounterClockwise : MTL::WindingClockwise;
    m_Fill = desc.rasterization.fillMode == FillMode::WIREFRAME ? MTL::TriangleFillModeLines : MTL::TriangleFillModeFill;
    m_DepthClip = desc.rasterization.depthClamp ? MTL::DepthClipModeClamp : MTL::DepthClipModeClip;
    m_DepthBounds = desc.outputMerger.depth.boundsTest;
    m_SampleLocations = desc.multisample && desc.multisample->sampleLocations;
    m_DepthBias = desc.rasterization.depthBias;

    return result;
}

DeviceMetal& PipelineMetal::GetDevice() const {
    return m_Device;
}

MTL::RenderPipelineState* PipelineMetal::GetRenderPipeline() const {
    return m_Render;
}

MTL::ComputePipelineState* PipelineMetal::GetComputePipeline() const {
    return m_Compute;
}

MTL::ResourceID PipelineMetal::GetVisibleFunctionTableResourceID() const {
    return m_VisibleFunctionTable ? m_VisibleFunctionTable->gpuResourceID() : MTL::ResourceID{};
}

MTL::ResourceID PipelineMetal::GetIntersectionFunctionTableResourceID() const {
    return m_IntersectionFunctionTable ? m_IntersectionFunctionTable->gpuResourceID() : MTL::ResourceID{};
}

Multiview PipelineMetal::GetMultiview() const {
    return m_Multiview;
}

uint32_t PipelineMetal::GetViewMask() const {
    return m_ViewMask;
}

MTL::DepthStencilState* PipelineMetal::GetDepthStencilState() const {
    return m_DepthStencil;
}

MTL::PrimitiveType PipelineMetal::GetPrimitiveType() const {
    return m_Primitive;
}

MTL::CullMode PipelineMetal::GetCullMode() const {
    return m_Cull;
}

MTL::Winding PipelineMetal::GetWinding() const {
    return m_Winding;
}

MTL::TriangleFillMode PipelineMetal::GetFillMode() const {
    return m_Fill;
}

MTL::DepthClipMode PipelineMetal::GetDepthClipMode() const {
    return m_DepthClip;
}

bool PipelineMetal::IsDepthBoundsEnabled() const {
    return m_DepthBounds;
}

bool PipelineMetal::HasSampleLocations() const {
    return m_SampleLocations;
}

const DepthBiasDesc& PipelineMetal::GetDepthBias() const {
    return m_DepthBias;
}

MTL::Size PipelineMetal::GetThreadGroupSize() const {
    return m_ThreadGroup;
}

MTL::Size PipelineMetal::GetMeshThreadGroupSize() const {
    return m_MeshGroup;
}

MTL::Size PipelineMetal::GetTaskThreadGroupSize() const {
    return m_TaskGroup;
}

OPENRHI_INLINE void PipelineMetal::SetDebugName(const char* name) {
    // Pipeline state labels are immutable after creation
    MaybeUnused(name);
}
