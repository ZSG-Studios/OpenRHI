enable_language(C)

if(OPENRHI_ENABLE_RMLUI_EXTENSION)
    # The bridge is qualified against the original public RmlUi headers/core, independent of the OpenRHI runtime
    set(RMLUI_FONT_ENGINE "none" CACHE STRING "RmlUi host-contract test font engine")
    set(RMLUI_SAMPLES OFF CACHE BOOL "RmlUi samples")
    set(RMLUI_PRECOMPILED_HEADERS OFF CACHE BOOL "RmlUi PCH")
    FetchContent_Declare(openrhi_rmlui_test
        URL "https://github.com/mikke89/RmlUi/archive/ba95ffe8bfb6370efb2cdcca927eaad4710c5413.zip"
        URL_HASH SHA256=ed3162ef67d24b7d91557d710b3734433cfc2a8659c55b4d68a11de38b178487
        DOWNLOAD_EXTRACT_TIMESTAMP 1)
    FetchContent_MakeAvailable(openrhi_rmlui_test)
    add_executable(OpenRHIRmlUiAdapter RmlUiAdapter.cpp)
    target_link_libraries(OpenRHIRmlUiAdapter PRIVATE OpenRHI RmlUi::Core)
    target_compile_features(OpenRHIRmlUiAdapter PRIVATE cxx_std_17)
    set_target_properties(OpenRHIRmlUiAdapter PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>")
    add_test(NAME OpenRHI.RmlUi.Adapter COMMAND OpenRHIRmlUiAdapter)
    set(ui_shader_outputs)
    foreach(target IN ITEMS spirv metal)
        foreach(stage IN ITEMS vs fs)
            if(stage STREQUAL "vs")
                set(entry RmlUiVS)
                set(kind vertex)
            else()
                set(entry RmlUiFS)
                set(kind fragment)
            endif()
            openrhi_compile_shader(NAME RmlUiContract_${target}_${stage}
                SOURCE "${PROJECT_SOURCE_DIR}/Shaders/RmlUi.slang" ENTRY ${entry} STAGE ${kind} TARGET ${target}
                INCLUDES "${PROJECT_SOURCE_DIR}/Include" OUTPUT binary)
            list(APPEND ui_shader_outputs "${binary}")
        endforeach()
    endforeach()
    add_custom_target(OpenRHIUIShaderContract ALL DEPENDS ${ui_shader_outputs})
endif()

if(OPENRHI_ENABLE_IMGUI_EXTENSION)
    FetchContent_Declare(openrhi_imgui_test
        URL "https://codeload.github.com/ocornut/imgui/zip/v1.92.9"
        URL_HASH SHA256=bbdcee6a1d56e38ccddaa58383182b348cfb4f2fc59f47a9ed9ef31f8f966351
        DOWNLOAD_EXTRACT_TIMESTAMP 1 SOURCE_SUBDIR "ignore CMakeLists.txt")
    FetchContent_MakeAvailable(openrhi_imgui_test)
    add_library(OpenRHIImguiTest STATIC
        "${openrhi_imgui_test_SOURCE_DIR}/imgui.cpp" "${openrhi_imgui_test_SOURCE_DIR}/imgui_draw.cpp"
        "${openrhi_imgui_test_SOURCE_DIR}/imgui_tables.cpp" "${openrhi_imgui_test_SOURCE_DIR}/imgui_widgets.cpp")
    target_include_directories(OpenRHIImguiTest PUBLIC "${openrhi_imgui_test_SOURCE_DIR}")
    target_compile_definitions(OpenRHIImguiTest PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
endif()

if(OPENRHI_ENABLE_IMGUI_EXTENSION AND OPENRHI_ENABLE_RMLUI_EXTENSION)
    add_executable(OpenRHIUIRender UIRender.cpp)
    target_link_libraries(OpenRHIUIRender PRIVATE OpenRHI OpenRHIImguiTest)
    target_compile_features(OpenRHIUIRender PRIVATE cxx_std_17)
    set_target_properties(OpenRHIUIRender PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>")
    foreach(backend IN ITEMS D3D12 VK METAL)
        if(OPENRHI_ENABLE_${backend}_SUPPORT AND NOT ANDROID AND NOT IOS)
            add_test(NAME OpenRHI.${backend}.UI COMMAND OpenRHIUIRender ${backend})
            set_tests_properties(OpenRHI.${backend}.UI PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
            if(APPLE)
                set_tests_properties(OpenRHI.${backend}.UI PROPERTIES ENVIRONMENT "MTL_DEBUG_LAYER=1;MTL_SHADER_VALIDATION=1")
            endif()
        endif()
    endforeach()
endif()
add_library(OpenRHICFacade OBJECT PublicFacade.c)
target_include_directories(OpenRHICFacade PRIVATE ../Include)
set_target_properties(OpenRHICFacade PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES)

add_executable(OpenRHIPublicAPI PublicApiTests.cpp $<TARGET_OBJECTS:OpenRHICFacade>)
target_link_libraries(OpenRHIPublicAPI PRIVATE OpenRHI)
target_compile_features(OpenRHIPublicAPI PRIVATE cxx_std_17)
target_compile_definitions(OpenRHIPublicAPI PRIVATE OPENRHI_TEST_NONE=$<BOOL:${OPENRHI_ENABLE_NONE_SUPPORT}>)
set_target_properties(OpenRHIPublicAPI PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>")
add_test(NAME OpenRHI.PublicAPI COMMAND OpenRHIPublicAPI)

add_executable(OpenRHISmoke BackendSmoke.cpp)
target_link_libraries(OpenRHISmoke PRIVATE OpenRHI)
target_compile_features(OpenRHISmoke PRIVATE cxx_std_17)
set_target_properties(OpenRHISmoke PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>"
)

foreach(backend IN ITEMS NONE D3D12 VK METAL)
    if(OPENRHI_ENABLE_${backend}_SUPPORT)
        add_test(NAME OpenRHI.${backend} COMMAND OpenRHISmoke ${backend})
        set_tests_properties(OpenRHI.${backend} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    endif()
endforeach()

add_test(NAME OpenRHI.SourceBoundary COMMAND ${CMAKE_COMMAND}
    -DROOT=${PROJECT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/SourceBoundary.cmake)

add_executable(OpenRHIDevicePolicy DevicePolicyTests.cpp)
target_include_directories(OpenRHIDevicePolicy PRIVATE ../Include ../Source/Shared)
target_compile_features(OpenRHIDevicePolicy PRIVATE cxx_std_17)
add_test(NAME OpenRHI.DevicePolicy COMMAND OpenRHIDevicePolicy)

add_executable(OpenRHITextureViewContract TextureViewContract.cpp)
target_include_directories(OpenRHITextureViewContract PRIVATE ../Source/Shared)
target_link_libraries(OpenRHITextureViewContract PRIVATE OpenRHI)
target_compile_features(OpenRHITextureViewContract PRIVATE cxx_std_17)
target_compile_definitions(OpenRHITextureViewContract PRIVATE OPENRHI_TEST_STATIC_LIBRARY=$<BOOL:${OPENRHI_STATIC_LIBRARY}>)
set_target_properties(OpenRHITextureViewContract PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>")
add_test(NAME OpenRHI.TextureViewContract COMMAND OpenRHITextureViewContract)

add_executable(OpenRHIUpstreamRegression UpstreamRegression.cpp)
target_link_libraries(OpenRHIUpstreamRegression PRIVATE OpenRHI)
target_compile_features(OpenRHIUpstreamRegression PRIVATE cxx_std_17)
set_target_properties(OpenRHIUpstreamRegression PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>")
foreach(backend IN ITEMS D3D12 VK METAL)
    if(OPENRHI_ENABLE_${backend}_SUPPORT AND NOT ANDROID AND NOT IOS)
        add_test(NAME OpenRHI.${backend}.UpstreamRegression COMMAND OpenRHIUpstreamRegression ${backend})
        set_tests_properties(OpenRHI.${backend}.UpstreamRegression PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        if(APPLE)
            set_tests_properties(OpenRHI.${backend}.UpstreamRegression PROPERTIES ENVIRONMENT "MTL_DEBUG_LAYER=1;MTL_SHADER_VALIDATION=1")
        endif()
    endif()
endforeach()

if(OPENRHI_ENABLE_VK_SUPPORT)
    add_library(OpenRHIVulkanInstanceCap SHARED VulkanInstanceCap.cpp)
    target_include_directories(OpenRHIVulkanInstanceCap PRIVATE "${vulkan_headers_SOURCE_DIR}/include")
    target_link_libraries(OpenRHIVulkanInstanceCap PRIVATE ${CMAKE_DL_LIBS})
    target_compile_features(OpenRHIVulkanInstanceCap PRIVATE cxx_std_17)
    set_target_properties(OpenRHIVulkanInstanceCap PROPERTIES OUTPUT_NAME vulkan
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/VulkanInstanceCap")
    if(NOT ANDROID)
        set_target_properties(OpenRHIVulkanInstanceCap PROPERTIES SOVERSION 1)
        find_file(OPENRHI_TEST_NATIVE_VULKAN_LOADER NAMES libvulkan.so.1 PATHS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
        if(OPENRHI_TEST_NATIVE_VULKAN_LOADER)
            add_test(NAME OpenRHI.VK.InstanceVersion COMMAND OpenRHIUpstreamRegression VK --instance-cap)
            set_tests_properties(OpenRHI.VK.InstanceVersion PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120
                ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:OpenRHIVulkanInstanceCap>;OPENRHI_TEST_NATIVE_VULKAN_LOADER=${OPENRHI_TEST_NATIVE_VULKAN_LOADER}")
        endif()
    endif()
endif()

if(EXISTS "${OPENRHI_ANF_SDK_ROOT}/public/anf.h")
    find_path(OPENRHI_ANF_TEST_VULKAN_HEADERS vulkan/vulkan.h HINTS "${vulkan_headers_SOURCE_DIR}/include" REQUIRED)
    add_executable(OpenRHIUpscalerAnfContract UpscalerAnfContract.cpp)
    target_include_directories(OpenRHIUpscalerAnfContract PRIVATE ../Include ../Source/Shared "${OPENRHI_ANF_SDK_ROOT}/public" "${OPENRHI_ANF_TEST_VULKAN_HEADERS}")
    target_compile_definitions(OpenRHIUpscalerAnfContract PRIVATE OPENRHI_STATIC_LIBRARY=1 OPENRHI_ENABLE_VK_SUPPORT=1 OPENRHI_ENABLE_ANF_SDK=1)
    target_compile_features(OpenRHIUpscalerAnfContract PRIVATE cxx_std_17)
    add_test(NAME OpenRHI.UpscalerAnfContract COMMAND OpenRHIUpscalerAnfContract)
endif()

if(OPENRHI_ENABLE_NGX_SDK AND OPENRHI_ENABLE_D3D12_SUPPORT)
    add_executable(OpenRHIUpscalerNgxLifecycle UpscalerNgxLifecycle.cpp)
    target_include_directories(OpenRHIUpscalerNgxLifecycle PRIVATE ../Include ../Source/Shared "${ngx_SOURCE_DIR}/include")
    target_compile_definitions(OpenRHIUpscalerNgxLifecycle PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
        OPENRHI_ENABLE_D3D12_SUPPORT=1 OPENRHI_ENABLE_NGX_SDK=1)
    target_compile_features(OpenRHIUpscalerNgxLifecycle PRIVATE cxx_std_17)
    add_test(NAME OpenRHI.UpscalerNgxLifecycle COMMAND OpenRHIUpscalerNgxLifecycle)
endif()

add_executable(OpenRHIQualification DeviceQualification.cpp)
target_link_libraries(OpenRHIQualification PRIVATE OpenRHI)
target_compile_features(OpenRHIQualification PRIVATE cxx_std_17)
set_target_properties(OpenRHIQualification PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>")
if(OPENRHI_ENABLE_D3D12_SUPPORT OR OPENRHI_ENABLE_VK_SUPPORT OR OPENRHI_ENABLE_METAL_SUPPORT)
    openrhi_compile_shader(NAME DescriptorCompute SOURCE DescriptorCompute.slang ENTRY descriptorMain STAGE compute
        INCLUDES "${PROJECT_SOURCE_DIR}/Include" OUTPUT QUALIFICATION_SHADER)
    openrhi_compile_shader(NAME IndirectDraw.vs SOURCE IndirectDraw.slang ENTRY vsMain STAGE vertex OUTPUT QUALIFICATION_VS)
    openrhi_compile_shader(NAME IndirectDraw.ps SOURCE IndirectDraw.slang ENTRY psMain STAGE fragment OUTPUT QUALIFICATION_PS)
    openrhi_compile_shader(NAME MeshDraw SOURCE MeshDraw.slang ENTRY meshMain STAGE mesh OUTPUT QUALIFICATION_MESH)
    openrhi_compile_shader(NAME TaskDraw SOURCE MeshDraw.slang ENTRY taskMain STAGE amplification OUTPUT QUALIFICATION_TASK)
    add_custom_target(OpenRHIQualificationShader DEPENDS "${QUALIFICATION_MESH}" "${QUALIFICATION_TASK}" "${QUALIFICATION_SHADER}" "${QUALIFICATION_VS}" "${QUALIFICATION_PS}")
    add_dependencies(OpenRHIQualification OpenRHIQualificationShader)
    if(WIN32)
        set(native_backend D3D12)
        set(presentation --present)
    elseif(APPLE)
        set(native_backend METAL)
    else()
        set(native_backend VK)
    endif()
    if(OPENRHI_ENABLE_NIS_SDK)
        set(nis_test --nis)
    endif()
    if(NOT ANDROID AND NOT IOS)
        add_test(NAME OpenRHI.${native_backend}.Resources COMMAND OpenRHIQualification --api ${native_backend}
            --iterations 8 --native-validation --shader "${QUALIFICATION_SHADER}" --vs "${QUALIFICATION_VS}" --ps "${QUALIFICATION_PS}" ${presentation} ${nis_test})
        set_tests_properties(OpenRHI.${native_backend}.Resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_test(NAME OpenRHI.${native_backend}.Mesh COMMAND OpenRHIQualification --api ${native_backend} --iterations 2
            --native-validation --mesh --vs "${QUALIFICATION_MESH}" --task "${QUALIFICATION_TASK}" --ps "${QUALIFICATION_PS}")
        set_tests_properties(OpenRHI.${native_backend}.Mesh PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        if(APPLE)
            set_tests_properties(OpenRHI.${native_backend}.Resources OpenRHI.${native_backend}.Mesh PROPERTIES ENVIRONMENT "MTL_DEBUG_LAYER=1;MTL_SHADER_VALIDATION=1")
        endif()
    endif()
endif()

add_executable(OpenRHIAdapterSelection AdapterSelection.cpp)
target_link_libraries(OpenRHIAdapterSelection PRIVATE OpenRHI)
set_target_properties(OpenRHIAdapterSelection PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>")
add_test(NAME OpenRHI.AdapterSelection COMMAND OpenRHIAdapterSelection)
set_tests_properties(OpenRHI.AdapterSelection PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)

if(WIN32 AND NOT OPENRHI_STATIC_LIBRARY)
    get_filename_component(OPENRHI_LINKER_DIR "${CMAKE_LINKER}" DIRECTORY)
    find_program(OPENRHI_DUMPBIN NAMES dumpbin HINTS "${OPENRHI_LINKER_DIR}")
    if(OPENRHI_DUMPBIN)
        add_test(NAME OpenRHI.WindowsBinaryBoundary COMMAND ${CMAKE_COMMAND}
            -DDUMPBIN=${OPENRHI_DUMPBIN} -DLIBRARY=$<TARGET_FILE:OpenRHI>
            -DD3D12=${OPENRHI_ENABLE_D3D12_SUPPORT}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/WindowsBinaryBoundary.cmake)
    endif()
endif()

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    if(OPENRHI_ENABLE_RMLUI_EXTENSION)
        add_test(NAME OpenRHI.RmlUi.Shaders COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_ui_shaders.py "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
    add_test(NAME OpenRHI.PromotionGate COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_qualification.py)
    if(NOT ANDROID AND NOT IOS AND NOT APPLE)
        add_test(NAME OpenRHI.SDKPackaging COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_sdk_packaging.py
            --root ${PROJECT_SOURCE_DIR} --output ${PROJECT_BINARY_DIR}/Tests/SDKPackaging)
    endif()
endif()

if(Python3_Interpreter_FOUND AND (OPENRHI_ENABLE_D3D12_SUPPORT OR OPENRHI_ENABLE_VK_SUPPORT OR OPENRHI_ENABLE_METAL_SUPPORT))
    add_test(NAME OpenRHI.SlangContract COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_shaders.py "${PROJECT_BINARY_DIR}")
endif()

if(OPENRHI_ENABLE_FFX_SDK AND OPENRHI_ENABLE_ANTILAG2_SDK AND OPENRHI_ENABLE_D3D12_SUPPORT)
    add_executable(OpenRHIVendorFeatures VendorFeatures.cpp)
    target_link_libraries(OpenRHIVendorFeatures PRIVATE OpenRHI d3d12 dxgi)
    target_include_directories(OpenRHIVendorFeatures PRIVATE
        "${fsr_SOURCE_DIR}/Kits/FidelityFX/api/include"
        "${fsr_SOURCE_DIR}/Kits/FidelityFX/denoisers/include"
        "${fsr_SOURCE_DIR}/Kits/FidelityFX/radiancecache/include"
        "${agilitysdk_SOURCE_DIR}/build/native/include")
    target_compile_features(OpenRHIVendorFeatures PRIVATE cxx_std_17)
    set_target_properties(OpenRHIVendorFeatures PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>")
    add_test(NAME OpenRHI.VendorFeatures.AMD COMMAND OpenRHIVendorFeatures)
    set_tests_properties(OpenRHI.VendorFeatures.AMD PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
endif()

if(OPENRHI_ENABLE_NRC_SDK)
    add_executable(OpenRHINeuralRadianceCacheContract NeuralRadianceCacheContract.cpp)
    target_include_directories(OpenRHINeuralRadianceCacheContract PRIVATE ../Include ../Source/Shared
        "${nrc_SOURCE_DIR}/Include" "${agilitysdk_SOURCE_DIR}/build/native/include")
    target_compile_definitions(OpenRHINeuralRadianceCacheContract PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX
        OPENRHI_STATIC_LIBRARY=1 OPENRHI_ENABLE_D3D12_SUPPORT=1 OPENRHI_ENABLE_NRC_SDK=1)
    target_compile_features(OpenRHINeuralRadianceCacheContract PRIVATE cxx_std_17)
    add_test(NAME OpenRHI.NeuralRadianceCacheContract COMMAND OpenRHINeuralRadianceCacheContract)
endif()

if(OPENRHI_ENABLE_NVAPI AND OPENRHI_ENABLE_D3D12_SUPPORT)
    find_path(OPENRHI_REFLEX_TEST_VULKAN_HEADERS vulkan/vulkan_core.h
        HINTS "${vulkan_headers_SOURCE_DIR}/include" "${OPENRHI_ANF_TEST_VULKAN_HEADERS}")
    if(OPENRHI_REFLEX_TEST_VULKAN_HEADERS)
        add_executable(OpenRHIReflexContract ReflexContract.cpp)
        target_include_directories(OpenRHIReflexContract PRIVATE ../Include ../Source/Shared
            "${nvapi_SOURCE_DIR}" "${OPENRHI_REFLEX_TEST_VULKAN_HEADERS}")
        target_compile_definitions(OpenRHIReflexContract PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX OPENRHI_STATIC_LIBRARY=1)
        target_compile_features(OpenRHIReflexContract PRIVATE cxx_std_17)
        add_test(NAME OpenRHI.ReflexContract COMMAND OpenRHIReflexContract)
    endif()
endif()

if(OPENRHI_ENABLE_NONE_SUPPORT)
    add_executable(OpenRHIUIAvailability UIAvailability.cpp)
    target_link_libraries(OpenRHIUIAvailability PRIVATE OpenRHI)
    target_compile_features(OpenRHIUIAvailability PRIVATE cxx_std_17)
    target_compile_definitions(OpenRHIUIAvailability PRIVATE OPENRHI_TEST_RMLUI=$<BOOL:${OPENRHI_ENABLE_RMLUI_EXTENSION}>)
    set_target_properties(OpenRHIUIAvailability PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>")
    add_test(NAME OpenRHI.RmlUi.Availability COMMAND OpenRHIUIAvailability)

    add_executable(OpenRHIVendorAvailability VendorAvailability.cpp)
    target_link_libraries(OpenRHIVendorAvailability PRIVATE OpenRHI)
    target_compile_features(OpenRHIVendorAvailability PRIVATE cxx_std_17)
    target_compile_definitions(OpenRHIVendorAvailability PRIVATE
        OPENRHI_TEST_FFX=$<AND:$<BOOL:${OPENRHI_ENABLE_FFX_SDK}>,$<BOOL:${OPENRHI_ENABLE_D3D12_SUPPORT}>>
        OPENRHI_TEST_NRC=$<BOOL:${OPENRHI_ENABLE_NRC_SDK}>
        OPENRHI_TEST_ANTILAG=$<BOOL:${OPENRHI_ENABLE_ANTILAG2_SDK}>)
    set_target_properties(OpenRHIVendorAvailability PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:OpenRHI>")
    add_test(NAME OpenRHI.VendorAvailability COMMAND OpenRHIVendorAvailability)
endif()
