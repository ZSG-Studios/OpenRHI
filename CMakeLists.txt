cmake_minimum_required(VERSION 3.30)

include(CMakeDependentOption)
include(FetchContent)

# Create project
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Include/OpenRHI.h" ver_h)
string(REGEX MATCH "OPENRHI_VERSION ([0-9]*)" _ ${ver_h})
set(OPENRHI_VERSION ${CMAKE_MATCH_1})

project(OpenRHI VERSION ${OPENRHI_VERSION} LANGUAGES CXX)
message("OpenRHI: v${OPENRHI_VERSION}:")
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "OpenRHI requires a 64-bit target")
endif()

# Test packages availability ("find_package" works only for X11)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_path(XLIB_HEADERS NAMES "X11/Xlib.h")
    find_path(WAYLAND_HEADERS NAMES "wayland-client.h")
endif()

# Architecture
if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64EC")
    set(IS_ARM64 FALSE)
    set(BIN_ARCHITECTURE "x64")
    set(NVAPI_BIN_ARCHITECTURE "amd64")
elseif((CMAKE_GENERATOR_PLATFORM MATCHES "ARM64") OR (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64"))
    set(IS_ARM64 TRUE)
    set(BIN_ARCHITECTURE "arm64")
    set(NVAPI_BIN_ARCHITECTURE "aarch64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(IS_ARM64 FALSE)
    set(BIN_ARCHITECTURE "x64")
    set(NVAPI_BIN_ARCHITECTURE "amd64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(IS_ARM64 FALSE)
    set(BIN_ARCHITECTURE "x86")
    set(NVAPI_BIN_ARCHITECTURE "x86")
else()
    message(FATAL_ERROR "OpenRHI: unsupported architecture or platform '${CMAKE_SYSTEM_NAME}', '${CMAKE_SYSTEM_PROCESSOR}'")
endif()

# Options
set(OPENRHI_AGILITY_SDK_DIR "AgilitySDK" CACHE STRING "Directory where Agility SDK will be copied relative to the directory with binaries")
set(OPENRHI_AGILITY_SDK_VERSION_MAJOR "619" CACHE STRING "Agility SDK major version") # 721 ("D3D12_ERROR_INVALID_REDIST" if Windows developer mode is not enabled)
set(OPENRHI_AGILITY_SDK_VERSION_MINOR "6" CACHE STRING "Agility SDK minor version") # 3-preview
set(OPENRHI_NVAPI_CUSTOM_PATH "" CACHE STRING "Path to a custom NVAPI library directory")

# A shared library on iOS requires framework packaging and code signing
if(IOS)
    set(OPENRHI_STATIC_LIBRARY_DEFAULT ON)
else()
    set(OPENRHI_STATIC_LIBRARY_DEFAULT OFF)
endif()

option(OPENRHI_STATIC_LIBRARY "Build static library" ${OPENRHI_STATIC_LIBRARY_DEFAULT})
option(OPENRHI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS "Enable debug names, host and device annotations" ON)
option(OPENRHI_ENABLE_NONE_SUPPORT "Enable NONE test backend" OFF)
option(OPENRHI_ENABLE_VALIDATION_SUPPORT "Enable Validation backend (otherwise 'enableOpenRHIValidation' is ignored)" ON)
option(OPENRHI_ENABLE_IMGUI_EXTENSION "Enable 'ImguiInterface' extension" OFF)
option(OPENRHI_STREAMER_THREAD_SAFE "'StreamerInterface' thread safety (OFF is faster)" ON)

# Metal 4 requires Xcode 26 or newer (AppleClang 17 or newer)
set(OPENRHI_METAL_SUPPORT_DEFAULT ON)
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)
    set(OPENRHI_METAL_SUPPORT_DEFAULT OFF)
    message(STATUS "OpenRHI: Metal 4 backend is disabled by default (requires Xcode 26 or newer)")
endif()

# Internal shaders are precompiled, which requires the Metal toolchain ("xcodebuild -downloadComponent MetalToolchain")
if(APPLE AND OPENRHI_METAL_SUPPORT_DEFAULT)
    if(IOS)
        set(OPENRHI_METAL_PROBE_SDK iphoneos)
    else()
        set(OPENRHI_METAL_PROBE_SDK macosx)
    endif()

    execute_process(COMMAND xcrun -sdk ${OPENRHI_METAL_PROBE_SDK} metal --version RESULT_VARIABLE OPENRHI_METAL_PROBE_RESULT OUTPUT_QUIET ERROR_QUIET)

    if(NOT OPENRHI_METAL_PROBE_RESULT EQUAL 0)
        set(OPENRHI_METAL_SUPPORT_DEFAULT OFF)
        message(STATUS "OpenRHI: Metal 4 backend is disabled by default (the Metal toolchain for '${OPENRHI_METAL_PROBE_SDK}' is not installed)")
    endif()
endif()

cmake_dependent_option(OPENRHI_ENABLE_D3D12_SUPPORT "Enable D3D12 backend" ON "WIN32" OFF)
cmake_dependent_option(OPENRHI_ENABLE_NVTX_SUPPORT "Annotations for NVIDIA Nsight Systems" ON "NOT IOS" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ANDROID)
    set(OPENRHI_VULKAN_PLATFORM ON)
else()
    set(OPENRHI_VULKAN_PLATFORM OFF)
endif()
option(OPENRHI_ENABLE_VK_SUPPORT "Enable Vulkan on Linux and Android" ${OPENRHI_VULKAN_PLATFORM})
if(OPENRHI_ENABLE_VK_SUPPORT AND NOT OPENRHI_VULKAN_PLATFORM)
    message(FATAL_ERROR "OpenRHI Vulkan requires Linux or Android; Windows uses D3D12 and Apple uses Metal")
endif()
if(ANDROID)
    # The NDK legacy toolchain uses CMAKE_SYSTEM_VERSION=1; its platform level is the target API
    if(DEFINED ANDROID_PLATFORM_LEVEL)
        set(OPENRHI_ANDROID_API "${ANDROID_PLATFORM_LEVEL}")
    elseif(DEFINED CMAKE_ANDROID_API)
        set(OPENRHI_ANDROID_API "${CMAKE_ANDROID_API}")
    else()
        set(OPENRHI_ANDROID_API "${CMAKE_SYSTEM_VERSION}")
    endif()
    if(OPENRHI_ANDROID_API LESS 33 OR NOT IS_ARM64)
        message(FATAL_ERROR "OpenRHI requires Android ARM64 API 33 or newer (target API ${OPENRHI_ANDROID_API})")
    endif()
endif()
option(OPENRHI_ENABLE_NIS_SDK "Enable the NVIDIA Image Scaling Slang upscaler" OFF)
cmake_dependent_option(OPENRHI_ENABLE_METAL_SUPPORT "Enable Metal 4 backend" ${OPENRHI_METAL_SUPPORT_DEFAULT} "APPLE" OFF)
if(OPENRHI_ENABLE_METAL_SUPPORT)
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "26.0" CACHE STRING "Minimum Apple OS version" FORCE)
    elseif(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 26)
        message(FATAL_ERROR "OpenRHI Metal 4 requires macOS/iOS/iPadOS 26 or newer")
    endif()
endif()
if(NOT OPENRHI_ENABLE_D3D12_SUPPORT AND NOT OPENRHI_ENABLE_VK_SUPPORT AND NOT OPENRHI_ENABLE_METAL_SUPPORT AND NOT OPENRHI_ENABLE_NONE_SUPPORT)
    message(FATAL_ERROR "OpenRHI requires a native backend or the explicit NONE test configuration")
endif()
cmake_dependent_option(OPENRHI_METAL_CPP_PRIVATE_IMPLEMENTATION "Compile metal-cpp implementation into OpenRHI (static OpenRHI: turn OFF if the app provides it)" ON "OPENRHI_ENABLE_METAL_SUPPORT; OPENRHI_STATIC_LIBRARY" ${OPENRHI_ENABLE_METAL_SUPPORT})
cmake_dependent_option(OPENRHI_ENABLE_AMDAGS "Enable AMD AGS library for D3D" ON "(OPENRHI_ENABLE_D3D12_SUPPORT); NOT IS_ARM64" OFF)
cmake_dependent_option(OPENRHI_ENABLE_NVAPI "Enable NVAPI library for D3D" ON "(OPENRHI_ENABLE_D3D12_SUPPORT)" OFF)
cmake_dependent_option(OPENRHI_ENABLE_AGILITY_SDK_SUPPORT "Enable Agility SDK support to unlock access to latest D3D12 features" ON "OPENRHI_ENABLE_D3D12_SUPPORT" OFF)
cmake_dependent_option(OPENRHI_ENABLE_XLIB_SUPPORT "Enable X11 support" ON "XLIB_HEADERS" OFF)
cmake_dependent_option(OPENRHI_ENABLE_WAYLAND_SUPPORT "Enable Wayland support" ON "WAYLAND_HEADERS" OFF)
cmake_dependent_option(OPENRHI_ENABLE_NGX_SDK "Enable NVIDIA NGX (DLSS) SDK" OFF "(OPENRHI_ENABLE_D3D12_SUPPORT OR OPENRHI_ENABLE_VK_SUPPORT); WIN32 OR NOT IS_ARM64" OFF)
cmake_dependent_option(OPENRHI_ENABLE_FFX_SDK "Enable AMD FidelityFX SDK" OFF "(OPENRHI_ENABLE_VK_SUPPORT OR OPENRHI_ENABLE_D3D12_SUPPORT); WIN32; NOT IS_ARM64" OFF)
cmake_dependent_option(OPENRHI_ENABLE_XESS_SDK "Enable INTEL XeSS SDK" OFF "OPENRHI_ENABLE_D3D12_SUPPORT; WIN32; NOT IS_ARM64" OFF)
cmake_dependent_option(OPENRHI_ENABLE_NRC_SDK "Enable NVIDIA NRC 0.16" OFF "WIN32; OPENRHI_ENABLE_D3D12_SUPPORT" OFF)
cmake_dependent_option(OPENRHI_ENABLE_ANTILAG2_SDK "Enable AMD Anti-Lag 2" OFF "WIN32; OPENRHI_ENABLE_D3D12_SUPPORT; NOT IS_ARM64" OFF)
cmake_dependent_option(OPENRHI_ENABLE_ANF_SDK "Enable Qualcomm ANF temporal SR" OFF "ANDROID; IS_ARM64; OPENRHI_ENABLE_VK_SUPPORT" OFF)
if(OPENRHI_ENABLE_ANF_SDK)
    set(OPENRHI_ANF_SDK_ROOT "" CACHE PATH "Unmodified ANF SDK 0.43.0 (a144fbab47ad586adf64c5a1574be73064550f1e)")
    foreach(input public/anf.h public/anf_types.h public/anf_types_vk.h public/anf_sr.h public/anf_fg.h lib/arm64-v8a/libanf.so LICENSE.txt LICENSE-BSD-3-Clause.txt)
        if(NOT EXISTS "${OPENRHI_ANF_SDK_ROOT}/${input}")
            message(FATAL_ERROR "ANF SDK input missing: ${OPENRHI_ANF_SDK_ROOT}/${input}")
        endif()
    endforeach()
    file(SHA256 "${OPENRHI_ANF_SDK_ROOT}/lib/arm64-v8a/libanf.so" OPENRHI_ANF_RUNTIME_SHA256)
    if(NOT OPENRHI_ANF_RUNTIME_SHA256 STREQUAL "284e8a1e0ec0a572eb56b1d06c778def9db0cd09e4ea20564caaa9612089384c")
        message(FATAL_ERROR "OpenRHI: ANF requires the unmodified SDK 0.43.0 runtime at a144fbab47ad586adf64c5a1574be73064550f1e")
    endif()
endif()

# Self-build?
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR}) # if not submodule
    get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if(IS_MULTI_CONFIG)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "")
    endif()

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/_Bin" CACHE STRING "") # .dll
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/_Bin" CACHE STRING "") # .so
endif()

# Enable grouping of source files into folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Compile definitions
set(COMPILE_DEFINITIONS "")
foreach(opt IN ITEMS
    OPENRHI_STATIC_LIBRARY
    OPENRHI_ENABLE_NVTX_SUPPORT
    OPENRHI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    OPENRHI_ENABLE_NONE_SUPPORT
    OPENRHI_ENABLE_VK_SUPPORT
    OPENRHI_ENABLE_METAL_SUPPORT
    OPENRHI_METAL_CPP_PRIVATE_IMPLEMENTATION
    OPENRHI_ENABLE_VALIDATION_SUPPORT
    OPENRHI_ENABLE_NIS_SDK
    OPENRHI_ENABLE_IMGUI_EXTENSION
    OPENRHI_ENABLE_D3D12_SUPPORT
    OPENRHI_ENABLE_NVAPI
    OPENRHI_ENABLE_AMDAGS
    OPENRHI_ENABLE_AGILITY_SDK_SUPPORT
    OPENRHI_ENABLE_XLIB_SUPPORT
    OPENRHI_ENABLE_WAYLAND_SUPPORT
    OPENRHI_ENABLE_NGX_SDK
    OPENRHI_ENABLE_FFX_SDK
    OPENRHI_ENABLE_XESS_SDK
    OPENRHI_ENABLE_ANF_SDK
    OPENRHI_ENABLE_NRC_SDK
    OPENRHI_ENABLE_ANTILAG2_SDK
    OPENRHI_STREAMER_THREAD_SAFE
)
    if(${opt})
        message(STATUS "${opt}")
        list(APPEND COMPILE_DEFINITIONS "${opt}=1")
    endif()
endforeach()

# Construct list of dependencies # TODO: update to latest releases from time to time
set(DEPS)

# Dependencies:
# - URL - good (no fat ".git" folder)
# - GIT_REPOSITORY - worse (because of ".git" folder)
# - GIT_SHALLOW = 0 - worse ^ 2 (needed to workaround Cmake's inability to shalow-clone a commit hash, which is a non-ToT PR, so even more fat useless ".git" folder)
if(OPENRHI_ENABLE_AGILITY_SDK_SUPPORT)
    # Agility SDK
    fetchcontent_declare(
        agilitysdk
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.${OPENRHI_AGILITY_SDK_VERSION_MAJOR}.${OPENRHI_AGILITY_SDK_VERSION_MINOR}"
    )
    list(APPEND DEPS agilitysdk)
endif()

if(OPENRHI_ENABLE_NVTX_SUPPORT)
    # NVTX
    fetchcontent_declare(
        nvtx
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL "https://github.com/NVIDIA/NVTX/archive/refs/tags/v3.6.0.zip"
    )
    list(APPEND DEPS nvtx)
endif()

if(OPENRHI_ENABLE_METAL_SUPPORT)
    # metal-cpp (macOS 27 SDK, the backend requires macOS 26+)
    fetchcontent_declare(
        metal_cpp
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL "https://github.com/apple/metal-cpp/archive/27c4382b7151d55a51692cdcb27aaa98752240de.zip"
    )
    list(APPEND DEPS metal_cpp)
endif()

if(OPENRHI_ENABLE_VK_SUPPORT)
    # Vulkan headers
    fetchcontent_declare(
        vulkan_headers
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL "https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.365.zip"
    )
    list(APPEND DEPS vulkan_headers)

    # Vulkan MA
    fetchcontent_declare(
        vma
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/3aa921224c154a0d2c43912bc88e1c42ce1f7607.zip"
    )
    list(APPEND DEPS vma)
endif()

set(NGX_VERSION "310.9.1")
if(IS_ARM64)
    set(NGX_WINDOWS_DIR "Windows_aarch64")
    set(NGX_WINDOWS_LIB_DIR "Windows_aarch64")
else()
    set(NGX_WINDOWS_DIR "Windows_x86_64")
    set(NGX_WINDOWS_LIB_DIR "Windows_x86_64/x64")
endif()

if(OPENRHI_ENABLE_NGX_SDK)
    # NVIDIA NGX SDK
    fetchcontent_declare(
        ngx
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL "https://github.com/NVIDIA/DLSS/archive/refs/tags/v${NGX_VERSION}.zip"
    )
    list(APPEND DEPS ngx)
endif()

if(OPENRHI_ENABLE_FFX_SDK)
    # Keep the legacy SDK for Vulkan (the newer FSR SDK only supports D3D12)
    if(OPENRHI_ENABLE_VK_SUPPORT)
        fetchcontent_declare(
            ffx
            DOWNLOAD_EXTRACT_TIMESTAMP 1
            DOWNLOAD_NO_PROGRESS 1
            SOURCE_SUBDIR "ignore CMakeLists.txt"
            URL "https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/releases/download/v1.1.4/FidelityFX-SDK-v1.1.4.zip"
        )
        list(APPEND DEPS ffx)
    endif()

    if(OPENRHI_ENABLE_D3D12_SUPPORT)
        fetchcontent_declare(
            fsr
            DOWNLOAD_EXTRACT_TIMESTAMP 1
            DOWNLOAD_NO_PROGRESS 1
            SOURCE_SUBDIR "ignore CMakeLists.txt"
            URL "https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/archive/refs/tags/v2.3.0.zip"
        )
        list(APPEND DEPS fsr)
    endif()
endif()

if(OPENRHI_ENABLE_NRC_SDK)
    fetchcontent_declare(nrc
        GIT_REPOSITORY "https://github.com/NVIDIA-RTX/NRC.git"
        GIT_TAG 7de30e4eb2a1a27118eb9a6acf5f715d757240c0
        SOURCE_SUBDIR "ignore CMakeLists.txt")
    list(APPEND DEPS nrc)
endif()
if(OPENRHI_ENABLE_ANTILAG2_SDK)
    fetchcontent_declare(antilag2
        URL "https://github.com/GPUOpen-LibrariesAndSDKs/AntiLag2-SDK/archive/390aa4a8c8655d0ae6e90079db2c85e103a96da3.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP 1 SOURCE_SUBDIR "ignore CMakeLists.txt")
    list(APPEND DEPS antilag2)
endif()

if(OPENRHI_ENABLE_XESS_SDK)
    # INTEL XeSS SDK
    fetchcontent_declare(
        xess
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL "https://github.com/intel/xess/archive/refs/tags/v3.0.2.zip"
    )
    list(APPEND DEPS xess)
endif()

if(OPENRHI_ENABLE_D3D12_SUPPORT)
    # D3D12 MA
    fetchcontent_declare(
        d3d12ma
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL "https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator/archive/1d86c1130f61453634b1df85782e1fecfd59a525.zip"
    )
    list(APPEND DEPS d3d12ma)
endif()

if(OPENRHI_ENABLE_AMDAGS)
    # AMD AGS
    fetchcontent_declare(
        amdags
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL "https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK/archive/refs/tags/v6.3.1.zip"
    )
    list(APPEND DEPS amdags)
endif()

if(OPENRHI_ENABLE_NVAPI)
    # NV API
    if(OPENRHI_NVAPI_CUSTOM_PATH)
        get_filename_component(nvapi_SOURCE_DIR "${OPENRHI_NVAPI_CUSTOM_PATH}" ABSOLUTE)
        message("OpenRHI: using custom NVAPI from '${nvapi_SOURCE_DIR}'")
    else()
        fetchcontent_declare(
            nvapi
            DOWNLOAD_EXTRACT_TIMESTAMP 1
            DOWNLOAD_NO_PROGRESS 1
            SOURCE_SUBDIR "ignore CMakeLists.txt"
            URL "https://github.com/NVIDIA/nvapi/archive/70d337db9186e968eab622f7e786de7e437faf3d.zip"
        )
        list(APPEND DEPS nvapi)
    endif()
endif()

# Download dependencies
if(DEPS)
    message("OpenRHI: downloading dependencies:")
    message(STATUS "${DEPS} ...")

    fetchcontent_makeavailable(${DEPS})


endif()

# NVAPI
if(OPENRHI_ENABLE_NVAPI)
    find_library(NVAPI_LIB
        NAMES
            nvapi64 nvapia64 nvapi
        PATHS
            "${nvapi_SOURCE_DIR}/${NVAPI_BIN_ARCHITECTURE}"
        REQUIRED
    ) # statically linked

    set(NVAPI_HEADERS
        "${nvapi_SOURCE_DIR}/nvapi.h"
        "${nvapi_SOURCE_DIR}/nvShaderExtnEnums.h"
    )
    source_group("External/nvapi"
        FILES
            ${NVAPI_HEADERS}
    )
endif()

# AMDAGS
if(OPENRHI_ENABLE_AMDAGS)
    find_library(AGS_LIB
        NAMES
            amd_ags_${BIN_ARCHITECTURE}
        PATHS
            "${amdags_SOURCE_DIR}/ags_lib/lib"
        REQUIRED
    ) # dynamically loaded

    set(AMDAGS_HEADERS
        "${amdags_SOURCE_DIR}/ags_lib/inc/amd_ags.h"
    )
    source_group("External/amdags"
        FILES
            ${AMDAGS_HEADERS}
    )
    set_source_files_properties(${AMDAGS_HEADERS}
        PROPERTIES
            VS_SETTINGS "ExcludedFromBuild=true" # shader compilation is explicit
    )
endif()

# Shared
if(OPENRHI_ENABLE_NGX_SDK)
    if(WIN32)
        if((NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY) OR (${CMAKE_MSVC_RUNTIME_LIBRARY} MATCHES "DLL"))
            set(NGX_SUFFIX "d")
            message("OpenRHI: using NGX LIB with dynamic runtime")
        else()
            set(NGX_SUFFIX "s")
            message("OpenRHI: using NGX LIB with static runtime")
        endif()

        set(NGX_LIB "${ngx_SOURCE_DIR}/lib/${NGX_WINDOWS_LIB_DIR}/nvsdk_ngx_${NGX_SUFFIX}$<$<CONFIG:Debug>:_dbg>.lib")
    else()
        set(NGX_LIB "${ngx_SOURCE_DIR}/lib/Linux_x86_64/libnvsdk_ngx.a")
    endif()
endif()

set(SHARED_SOURCE
    "Source/OpenRHIConfig.h"
    "Source/Shared/DeviceBase.h"
    "Source/Shared/DevicePolicy.h"
    "Source/Shared/HelperInterface.h"
    "Source/Shared/HelperInterface.hpp"
    "Source/Shared/ImguiInterface.h"
    "Source/Shared/ImguiInterface.hpp"
    "Source/Shared/Lock.h"
    "Source/Shared/Latency.h"
    "Source/Shared/NIS.h"
    "Source/Shared/Shared.cpp"
    "Source/Shared/SharedExternal.h"
    "Source/Shared/SharedExternal.hpp"
    "Source/Shared/SharedLibrary.hpp"
    "Source/Shared/StreamerInterface.h"
    "Source/Shared/StreamerInterface.hpp"
    "Source/Shared/UpscalerInterface.h"
    "Source/Shared/UpscalerInterface.hpp"
    "Source/Shared/VendorFeatures.h"
    "Source/Shared/VendorFeaturesNative.h"
    "Source/Shared/VendorFeatures.hpp"
    "Source/Shared/AdrenoNeuralFusion.h"
    "Source/Shared/AdrenoNeuralFusion.hpp"
    "Source/Shared/SharedVideo.h"
)

add_library(OpenRHI_Shared STATIC)
target_sources(OpenRHI_Shared
    PRIVATE
        ${SHARED_SOURCE}
)
target_include_directories(OpenRHI_Shared
    PUBLIC
        "Include"
        "Source/Shared"
    PRIVATE
        $<$<BOOL:${OPENRHI_ENABLE_AGILITY_SDK_SUPPORT}>:
            ${agilitysdk_SOURCE_DIR}/build/native/include
        >
        $<$<BOOL:${OPENRHI_ENABLE_VK_SUPPORT}>:
            ${vulkan_headers_SOURCE_DIR}/include
        >
        $<$<BOOL:${OPENRHI_ENABLE_NGX_SDK}>:
            ${ngx_SOURCE_DIR}/include
        >
        $<$<BOOL:${OPENRHI_ENABLE_ANF_SDK}>:
            ${OPENRHI_ANF_SDK_ROOT}/public
        >
        $<$<AND:$<BOOL:${OPENRHI_ENABLE_FFX_SDK}>,$<BOOL:${OPENRHI_ENABLE_D3D12_SUPPORT}>>:
            ${fsr_SOURCE_DIR}/Kits/FidelityFX/api/include
            ${fsr_SOURCE_DIR}/Kits/FidelityFX/upscalers/include
            ${fsr_SOURCE_DIR}/Kits/FidelityFX/denoisers/include
            ${fsr_SOURCE_DIR}/Kits/FidelityFX/radiancecache/include
        >
        $<$<AND:$<BOOL:${OPENRHI_ENABLE_FFX_SDK}>,$<NOT:$<BOOL:${OPENRHI_ENABLE_D3D12_SUPPORT}>>>:
            ${ffx_SOURCE_DIR}/ffx-api/include/ffx_api
        >
        $<$<BOOL:${OPENRHI_ENABLE_NRC_SDK}>:${nrc_SOURCE_DIR}/Include>
        $<$<BOOL:${OPENRHI_ENABLE_ANTILAG2_SDK}>:${antilag2_SOURCE_DIR}>
        $<$<BOOL:${OPENRHI_ENABLE_XESS_SDK}>:
            ${xess_SOURCE_DIR}/inc/xess
        >
)
target_compile_definitions(OpenRHI_Shared
    PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        ${COMPILE_DEFINITIONS}
)
target_compile_options(OpenRHI_Shared
    PUBLIC
        # Options for Clang/GCC compilers
        # No "nullability-completeness" caused by VMA
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>:
                -Wextra
                -Werror
                -fvisibility=hidden
            >
            -Wno-missing-field-initializers
            -Wno-nullability-completeness
        >
        # Options for MSVC frontend
        $<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:
            /W4
            /WX
            /wd4324
            $<$<CONFIG:Release>:/Zi>
        >
)
target_compile_features(OpenRHI_Shared
    PUBLIC
        cxx_std_17
)
target_link_libraries(OpenRHI_Shared
    PRIVATE
        $<$<BOOL:${OPENRHI_ENABLE_NGX_SDK}>:
            ${NGX_LIB}
        >
        $<$<BOOL:${OPENRHI_ENABLE_XESS_SDK}>:
            ${xess_SOURCE_DIR}/lib/libxess.lib
        >
)
set_target_properties(OpenRHI_Shared
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON           # enable PIC...
        INTERFACE_POSITION_INDEPENDENT_CODE ON # ...and propagate to all consumers
        FOLDER "OpenRHI"
)

if(OPENRHI_ENABLE_NRC_SDK)
    if(IS_ARM64)
        set(OPENRHI_NRC_ARCH arm64)
    else()
        set(OPENRHI_NRC_ARCH x64)
    endif()
    foreach(backend IN ITEMS D3D12 VK)
        if(OPENRHI_ENABLE_${backend}_SUPPORT)
            if(backend STREQUAL "VK")
                set(nrc_native Vulkan)
            else()
                set(nrc_native D3D12)
            endif()
            target_link_libraries(OpenRHI_Shared PRIVATE "${nrc_SOURCE_DIR}/Lib/${OPENRHI_NRC_ARCH}/NRC_${nrc_native}.lib")
        endif()
    endforeach()
endif()

# NONE
if(OPENRHI_ENABLE_NONE_SUPPORT)
    set(NONE_SOURCE "Source/NONE/ImplNONE.cpp")

    add_library(OpenRHI_NONE STATIC)
    target_sources(OpenRHI_NONE
        PRIVATE
            ${NONE_SOURCE}
    )
    target_link_libraries(OpenRHI_NONE
        PRIVATE
            OpenRHI_Shared
    )
    set_target_properties(OpenRHI_NONE
        PROPERTIES
            FOLDER "OpenRHI"
    )
endif()

# D3D12
if(OPENRHI_ENABLE_D3D12_SUPPORT)
    message("OpenRHI: using Windows SDK ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

    set(D3D12_VMA "${d3d12ma_SOURCE_DIR}/include/D3D12MemAlloc.h")
    source_group("External/Vma"
        FILES
            ${D3D12_VMA}
    )

    set(D3D12_SOURCE
        "Source/D3D12/AccelerationStructureD3D12.h"
        "Source/D3D12/AccelerationStructureD3D12.hpp"
        "Source/D3D12/BufferD3D12.h"
        "Source/D3D12/BufferD3D12.hpp"
        "Source/D3D12/CommandAllocatorD3D12.h"
        "Source/D3D12/CommandAllocatorD3D12.hpp"
        "Source/D3D12/CommandBufferD3D12.h"
        "Source/D3D12/CommandBufferD3D12.hpp"
        "Source/D3D12/DescriptorD3D12.h"
        "Source/D3D12/DescriptorD3D12.hpp"
        "Source/D3D12/DescriptorHeapD3D12.h"
        "Source/D3D12/DescriptorHeapD3D12.hpp"
        "Source/D3D12/DescriptorPoolD3D12.h"
        "Source/D3D12/DescriptorPoolD3D12.hpp"
        "Source/D3D12/DescriptorSetD3D12.h"
        "Source/D3D12/DescriptorSetD3D12.hpp"
        "Source/D3D12/DeviceD3D12.h"
        "Source/D3D12/DeviceD3D12.hpp"
        "Source/D3D12/FenceD3D12.h"
        "Source/D3D12/FenceD3D12.hpp"
        "Source/D3D12/ImplD3D12.cpp"
        "Source/D3D12/MemoryAllocatorD3D12.h"
        "Source/D3D12/MemoryD3D12.h"
        "Source/D3D12/MemoryD3D12.hpp"
        "Source/D3D12/MicromapD3D12.h"
        "Source/D3D12/MicromapD3D12.hpp"
        "Source/D3D12/PipelineCacheD3D12.h"
        "Source/D3D12/PipelineCacheD3D12.hpp"
        "Source/D3D12/PipelineD3D12.h"
        "Source/D3D12/PipelineD3D12.hpp"
        "Source/D3D12/PipelineLayoutD3D12.h"
        "Source/D3D12/PipelineLayoutD3D12.hpp"
        "Source/D3D12/QueryPoolD3D12.h"
        "Source/D3D12/QueryPoolD3D12.hpp"
        "Source/D3D12/QueueD3D12.h"
        "Source/D3D12/QueueD3D12.hpp"
        "Source/D3D12/SharedD3D12.h"
        "Source/D3D12/SharedD3D12.hpp"
        "Source/D3D12/SwapChainD3D12.h"
        "Source/D3D12/SwapChainD3D12.hpp"
        "Source/D3D12/TextureD3D12.h"
        "Source/D3D12/TextureD3D12.hpp"
        "Source/D3D12/TransferContextD3D12.h"
        "Source/D3D12/TransferContextD3D12.hpp"
        "Source/D3D12/VideoHelpersD3D12.h"
        "Source/D3D12/VideoPictureD3D12.h"
        "Source/D3D12/VideoPictureD3D12.hpp"
        "Source/D3D12/VideoSessionD3D12.h"
        "Source/D3D12/VideoSessionD3D12.hpp"
        "Source/D3D12/VideoSessionParametersD3D12.h"
        "Source/D3D12/VideoSessionParametersD3D12.hpp"
    )

    add_library(OpenRHI_D3D12 STATIC)
    target_sources(OpenRHI_D3D12
        PRIVATE
            ${D3D12_SOURCE}
            ${D3D12_VMA}
            $<$<BOOL:${OPENRHI_ENABLE_NVAPI}>:
                ${NVAPI_HEADERS}
            >
            $<$<BOOL:${OPENRHI_ENABLE_AMDAGS}>:
                ${AMDAGS_HEADERS}
            >
    )
    target_include_directories(OpenRHI_D3D12
        PRIVATE
            "${d3d12ma_SOURCE_DIR}/include"
            "${d3d12ma_SOURCE_DIR}/src"
            $<$<BOOL:${OPENRHI_ENABLE_NVAPI}>:
                "${nvapi_SOURCE_DIR}"
            >
            $<$<BOOL:${OPENRHI_ENABLE_AMDAGS}>:
                "${amdags_SOURCE_DIR}/ags_lib/inc"
            >
            $<$<BOOL:${OPENRHI_ENABLE_AGILITY_SDK_SUPPORT}>:
                ${agilitysdk_SOURCE_DIR}/build/native/include
            >
    )
    target_compile_definitions(OpenRHI_D3D12
        PRIVATE
            OPENRHI_AGILITY_SDK_VERSION_MAJOR=$<IF:$<BOOL:${OPENRHI_ENABLE_AGILITY_SDK_SUPPORT}>,${OPENRHI_AGILITY_SDK_VERSION_MAJOR},D3D12_SDK_VERSION>
    )
    target_link_libraries(OpenRHI_D3D12
        PRIVATE
            OpenRHI_Shared
            d3d12
            dxgi
            dxguid
            $<$<BOOL:${OPENRHI_ENABLE_NVAPI}>:
                ${NVAPI_LIB}
            >
    )
    set_target_properties(OpenRHI_D3D12
        PROPERTIES
            FOLDER "OpenRHI"
    )

    if(OPENRHI_ENABLE_AGILITY_SDK_SUPPORT)
        message("OpenRHI: using AgilitySDK v1.${OPENRHI_AGILITY_SDK_VERSION_MAJOR}.${OPENRHI_AGILITY_SDK_VERSION_MINOR}")

        # Generate "OpenRHIAgilitySDK.h"
        file(WRITE Include/OpenRHIAgilitySDK.h
            "// This file is auto-generated during project deployment. Do not modify!\n"
            "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif\n\n"
            "__declspec(dllexport) extern const uint32_t D3D12SDKVersion;\n"
            "const uint32_t D3D12SDKVersion = ${OPENRHI_AGILITY_SDK_VERSION_MAJOR};\n\n"
            "__declspec(dllexport) extern const char* D3D12SDKPath;\n"
            "const char* D3D12SDKPath = \"${OPENRHI_AGILITY_SDK_DIR}/\";\n\n"
            "#ifdef __cplusplus\n"
            "}\n"
            "#endif\n"
        )
    endif()
endif()

# VK
if(OPENRHI_ENABLE_VK_SUPPORT)

    set(VK_VMA "${vma_SOURCE_DIR}/include/vk_mem_alloc.h")
    source_group("External/Vma"
        FILES
            ${VK_VMA}
    )

    set(VK_SOURCE
        "Source/VK/AccelerationStructureVK.h"
        "Source/VK/AccelerationStructureVK.hpp"
        "Source/VK/BufferVK.h"
        "Source/VK/BufferVK.hpp"
        "Source/VK/CommandAllocatorVK.h"
        "Source/VK/CommandAllocatorVK.hpp"
        "Source/VK/CommandBufferVK.h"
        "Source/VK/CommandBufferVK.hpp"
        "Source/VK/ConversionVK.h"
        "Source/VK/ConversionVK.hpp"
        "Source/VK/DescriptorPoolVK.h"
        "Source/VK/DescriptorPoolVK.hpp"
        "Source/VK/DescriptorSetVK.h"
        "Source/VK/DescriptorSetVK.hpp"
        "Source/VK/DescriptorVK.h"
        "Source/VK/DescriptorVK.hpp"
        "Source/VK/DescriptorHeapVK.h"
        "Source/VK/DescriptorHeapVK.hpp"
        "Source/VK/DeviceVK.h"
        "Source/VK/DeviceVK.hpp"
        "Source/VK/DispatchTable.h"
        "Source/VK/FenceVK.h"
        "Source/VK/FenceVK.hpp"
        "Source/VK/ImplVK.cpp"
        "Source/VK/MemoryAllocatorVK.h"
        "Source/VK/MemoryVK.h"
        "Source/VK/MemoryVK.hpp"
        "Source/VK/MicromapVK.h"
        "Source/VK/MicromapVK.hpp"
        "Source/VK/PipelineCacheVK.h"
        "Source/VK/PipelineCacheVK.hpp"
        "Source/VK/PipelineLayoutVK.h"
        "Source/VK/PipelineLayoutVK.hpp"
        "Source/VK/PipelineVK.h"
        "Source/VK/PipelineVK.hpp"
        "Source/VK/QueryPoolVK.h"
        "Source/VK/QueryPoolVK.hpp"
        "Source/VK/QueueVK.h"
        "Source/VK/QueueVK.hpp"
        "Source/VK/SharedVK.h"
        "Source/VK/SwapChainVK.h"
        "Source/VK/SwapChainVK.hpp"
        "Source/VK/TextureVK.h"
        "Source/VK/TextureVK.hpp"
        "Source/VK/TransferContextVK.h"
        "Source/VK/TransferContextVK.hpp"
        "Source/VK/VideoHelpersVK.h"
        "Source/VK/VideoPictureVK.h"
        "Source/VK/VideoPictureVK.hpp"
        "Source/VK/VideoSessionVK.h"
        "Source/VK/VideoSessionVK.hpp"
        "Source/VK/VideoSessionParametersVK.h"
        "Source/VK/VideoSessionParametersVK.hpp"
    )

    add_library(OpenRHI_VK STATIC)
    target_sources(OpenRHI_VK
        PRIVATE
            ${VK_SOURCE}
            ${VK_VMA}
    )
    target_include_directories(OpenRHI_VK
        PRIVATE
            "${vma_SOURCE_DIR}/include"
            "${vulkan_headers_SOURCE_DIR}/include"
    )
    target_compile_definitions(OpenRHI_VK
        PRIVATE
            $<$<BOOL:${ANDROID}>:
                VK_USE_PLATFORM_ANDROID_KHR
            >
            $<$<BOOL:${OPENRHI_ENABLE_XLIB_SUPPORT}>:
                VK_USE_PLATFORM_XLIB_KHR
            >
            $<$<BOOL:${OPENRHI_ENABLE_WAYLAND_SUPPORT}>:
                VK_USE_PLATFORM_WAYLAND_KHR
            >
    )
    target_link_libraries(OpenRHI_VK
        PRIVATE
            OpenRHI_Shared
    )
    set_target_properties(OpenRHI_VK
        PROPERTIES
            FOLDER "OpenRHI"
    )
endif()

# Metal
if(OPENRHI_ENABLE_METAL_SUPPORT)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)
        message(FATAL_ERROR "OpenRHI: Metal 4 requires Xcode 26 or newer (AppleClang 17 or newer)")
    endif()

    if(NOT EXISTS "${metal_cpp_SOURCE_DIR}/Metal/Metal.hpp" OR NOT EXISTS "${metal_cpp_SOURCE_DIR}/Metal/MTL4ArgumentTable.hpp")
        message(FATAL_ERROR "OpenRHI: Metal 4 metal-cpp headers were not found at '${metal_cpp_SOURCE_DIR}'")
    endif()


    # MetalFX is not in the iOS simulator SDK (with the Xcode generator "CMAKE_OSX_SYSROOT" must match the SDK used for building)
    if(IOS AND CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
        set(OPENRHI_METALFX_FRAMEWORK "")
    else()
        set(OPENRHI_METALFX_FRAMEWORK "-framework MetalFX")
    endif()

    include(CMake/OpenRHISlang.cmake)
    set(internal_groups CLEAR BUFFER RESOLVE_VS FILTER ROOTS DEPTH INSTANCES HEADER WORDS)
    foreach(dimension RANGE 0 5)
        foreach(type RANGE 0 2)
            list(APPEND internal_groups STORAGE_${dimension}_${type})
        endforeach()
    endforeach()
    foreach(dimension IN ITEMS 2d array)
        foreach(type RANGE 0 2)
            list(APPEND internal_groups RESOLVE_${dimension}_${type})
        endforeach()
    endforeach()
    set(internal_air)
    foreach(group IN LISTS internal_groups)
        openrhi_compile_shader(NAME Internal_${group} SOURCE Shaders/InternalMetal.slang
            DEFINES OPENRHI_INTERNAL_${group}=1 AIR air)
        list(APPEND internal_air "${air}")
    endforeach()
    find_program(OPENRHI_XCRUN xcrun REQUIRED)
    if(IOS)
        if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
            set(internal_sdk iphonesimulator)
        else()
            set(internal_sdk iphoneos)
        endif()
    else()
        set(internal_sdk macosx)
    endif()
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/InternalMetal.metallib.h"
        COMMAND "${OPENRHI_XCRUN}" -sdk ${internal_sdk} metallib ${internal_air} -o "${CMAKE_CURRENT_BINARY_DIR}/InternalMetal.metallib"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${CMAKE_CURRENT_BINARY_DIR}/InternalMetal.metallib"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/InternalMetal.metallib.h" -DNAME=InternalMetal_metallib
            -P "${CMAKE_CURRENT_SOURCE_DIR}/CMake/EmbedShader.cmake"
        DEPENDS ${internal_air} "${CMAKE_CURRENT_SOURCE_DIR}/CMake/EmbedShader.cmake" VERBATIM)
    add_custom_target(OpenRHI_InternalShaders DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/InternalMetal.metallib.h")

    set(METAL_SOURCE
        "Source/Metal/AccelerationStructureMetal.h"
        "Source/Metal/AccelerationStructureMetal.hpp"
        "Source/Metal/BufferMetal.h"
        "Source/Metal/BufferMetal.hpp"
        "Source/Metal/CommandAllocatorMetal.h"
        "Source/Metal/CommandAllocatorMetal.hpp"
        "Source/Metal/CommandBufferMetal.h"
        "Source/Metal/CommandBufferMetal.hpp"
        "Source/Metal/ConversionMetal.h"
        "Source/Metal/ConversionMetal.hpp"
        "Source/Metal/DescriptorMetal.h"
        "Source/Metal/DescriptorMetal.hpp"
        "Source/Metal/DescriptorPoolMetal.h"
        "Source/Metal/DescriptorPoolMetal.hpp"
        "Source/Metal/DescriptorSetMetal.h"
        "Source/Metal/DescriptorSetMetal.hpp"
        "Source/Metal/DeviceMetal.h"
        "Source/Metal/DeviceMetal.hpp"
        "Source/Metal/FenceMetal.h"
        "Source/Metal/FenceMetal.hpp"
        "Source/Metal/ImplMetal.cpp"
        "Source/Metal/InternalShadersMetal.h"
        "Source/Metal/InternalShadersMetal.hpp"
        "Source/Metal/MemoryMetal.h"
        "Source/Metal/MemoryMetal.hpp"
        "Source/Metal/PipelineCacheMetal.h"
        "Source/Metal/PipelineCacheMetal.hpp"
        "Source/Metal/PipelineLayoutMetal.h"
        "Source/Metal/PipelineLayoutMetal.hpp"
        "Source/Metal/PipelineMetal.h"
        "Source/Metal/PipelineMetal.hpp"
        "Source/Metal/QueryPoolMetal.h"
        "Source/Metal/QueryPoolMetal.hpp"
        "Source/Metal/QueueMetal.h"
        "Source/Metal/QueueMetal.hpp"
        "Source/Metal/SharedMetal.h"
        "Source/Metal/SwapChainMetal.h"
        "Source/Metal/SwapChainMetal.hpp"
        "Source/Metal/TextureMetal.h"
        "Source/Metal/TextureMetal.hpp"
    )

    target_include_directories(OpenRHI_Shared PRIVATE "${metal_cpp_SOURCE_DIR}") # MetalFX in "UpscalerInterface.hpp"

    add_library(OpenRHI_Metal STATIC)
    target_sources(OpenRHI_Metal
        PRIVATE
            ${METAL_SOURCE}
    )
    target_include_directories(OpenRHI_Metal
        PRIVATE
            "${metal_cpp_SOURCE_DIR}"
    )
    target_link_libraries(OpenRHI_Metal
        PRIVATE
            OpenRHI_Shared
            "-framework Foundation"
            "-framework Metal"
            ${OPENRHI_METALFX_FRAMEWORK}
            "-framework QuartzCore"
            "-framework CoreGraphics"
    )
    set_target_properties(OpenRHI_Metal
        PROPERTIES
            FOLDER "OpenRHI"
    )

    add_dependencies(OpenRHI_Metal OpenRHI_InternalShaders)
    target_include_directories(OpenRHI_Metal PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
endif()

# Validation
if(OPENRHI_ENABLE_VALIDATION_SUPPORT)
    set(OPENRHI_VALIDATION_SOURCE
        "Source/Validation/AccelerationStructureVal.h"
        "Source/Validation/AccelerationStructureVal.hpp"
        "Source/Validation/BufferVal.h"
        "Source/Validation/BufferVal.hpp"
        "Source/Validation/CommandAllocatorVal.h"
        "Source/Validation/CommandAllocatorVal.hpp"
        "Source/Validation/CommandBufferVal.h"
        "Source/Validation/CommandBufferVal.hpp"
        "Source/Validation/ConversionVal.hpp"
        "Source/Validation/DescriptorPoolVal.h"
        "Source/Validation/DescriptorPoolVal.hpp"
        "Source/Validation/DescriptorHeapVal.h"
        "Source/Validation/DescriptorHeapVal.hpp"
        "Source/Validation/DescriptorSetVal.h"
        "Source/Validation/DescriptorSetVal.hpp"
        "Source/Validation/DescriptorVal.h"
        "Source/Validation/DescriptorVal.hpp"
        "Source/Validation/DeviceVal.h"
        "Source/Validation/DeviceVal.hpp"
        "Source/Validation/FenceVal.h"
        "Source/Validation/FenceVal.hpp"
        "Source/Validation/ImplVal.cpp"
        "Source/Validation/MemoryVal.h"
        "Source/Validation/MemoryVal.hpp"
        "Source/Validation/MicromapVal.h"
        "Source/Validation/MicromapVal.hpp"
        "Source/Validation/PipelineCacheVal.h"
        "Source/Validation/PipelineCacheVal.hpp"
        "Source/Validation/PipelineLayoutVal.h"
        "Source/Validation/PipelineLayoutVal.hpp"
        "Source/Validation/PipelineVal.h"
        "Source/Validation/PipelineVal.hpp"
        "Source/Validation/QueryPoolVal.h"
        "Source/Validation/QueryPoolVal.hpp"
        "Source/Validation/QueueVal.h"
        "Source/Validation/QueueVal.hpp"
        "Source/Validation/SharedVal.h"
        "Source/Validation/SwapChainVal.h"
        "Source/Validation/SwapChainVal.hpp"
        "Source/Validation/VendorFeaturesVal.h"
        "Source/Validation/VendorFeaturesVal.hpp"
        "Source/Validation/TextureVal.h"
        "Source/Validation/TextureVal.hpp"
        "Source/Validation/VideoPictureVal.h"
        "Source/Validation/VideoPictureVal.hpp"
        "Source/Validation/VideoSessionParametersVal.h"
        "Source/Validation/VideoSessionParametersVal.hpp"
        "Source/Validation/VideoSessionVal.h"
        "Source/Validation/VideoSessionVal.hpp"
    )

    add_library(OpenRHI_Validation STATIC)
    target_sources(OpenRHI_Validation
        PRIVATE
            ${OPENRHI_VALIDATION_SOURCE}
    )
    target_link_libraries(OpenRHI_Validation
        PRIVATE
            OpenRHI_Shared
    )
    set_target_properties(OpenRHI_Validation
        PROPERTIES
            FOLDER "OpenRHI"
    )
endif()

# Core headers
set(OPENRHI_HEADERS
    "Include/OpenRHI.h"
    "Include/OpenRHI.slang"
    "Include/OpenRHIDescs.h"
    "Include/OpenRHIMacro.h"
)
source_group("Include"
    FILES
        ${OPENRHI_HEADERS}
)
set_source_files_properties(${OPENRHI_HEADERS}
    PROPERTIES
        VS_SETTINGS "ExcludedFromBuild=true" # shader compilation is explicit
)
set_source_files_properties("Include/OpenRHI.slang"
    PROPERTIES
        HEADER_FILE_ONLY TRUE # included by Slang
)

# Extensions headers
set(OPENRHI_EXTENSIONS
    "Include/Extensions/OpenRHIDescriptorHeap.h"
    "Include/Extensions/OpenRHIDeviceCreation.h"
    "Include/Extensions/OpenRHIHelper.h"
    "Include/Extensions/OpenRHIImgui.h"
    "Include/Extensions/OpenRHILowLatency.h"
    "Include/Extensions/OpenRHIMeshShader.h"
    "Include/Extensions/OpenRHIRayTracing.h"
    "Include/Extensions/OpenRHIStreamer.h"
    "Include/Extensions/OpenRHISwapChain.h"
    "Include/Extensions/OpenRHIUpscaler.h"
    "Include/Extensions/OpenRHIVendorFeatures.h"
    "Include/Extensions/OpenRHIUpscalerVK.h"
    "Include/Extensions/OpenRHIVideo.h"
    "Include/Extensions/OpenRHIWrapperD3D12.h"
    "Include/Extensions/OpenRHIWrapperMetal.h"
    "Include/Extensions/OpenRHIWrapperVK.h"
)
source_group("Include/Extensions"
    FILES
        ${OPENRHI_EXTENSIONS}
)

# Resources
set(OPENRHI_RESOURCES
    "Resources/OpenRHI.rc"
    "Resources/Version.h"
)
source_group("Resources"
    FILES
        ${OPENRHI_RESOURCES}
)

# OpenRHI
set(OPENRHI_SOURCE "Source/Creation/Creation.cpp")
source_group("Sources"
    FILES
        ${OPENRHI_SOURCE}
)

if(OPENRHI_STATIC_LIBRARY)
    add_library(OpenRHI STATIC)
else()
    add_library(OpenRHI SHARED)

    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_link_options(OpenRHI
            PRIVATE
                $<$<CONFIG:Release>:/DEBUG> # generate a PDB for Release build
                $<$<CONFIG:Release>:/OPT:ICF> # perform identical COMDAT folding (removes duplicate code/data)
        )
    endif()
endif()

target_sources(OpenRHI
    PRIVATE
        ${OPENRHI_SOURCE}
        ${OPENRHI_HEADERS}
        ${OPENRHI_RESOURCES}
        ${OPENRHI_EXTENSIONS}
        $<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:openrhi.natvis> # debugger markup
)
target_include_directories(OpenRHI
    PRIVATE
        $<$<BOOL:${OPENRHI_ENABLE_NVTX_SUPPORT}>:
            "${nvtx_SOURCE_DIR}/c/include/"
        >
        $<$<BOOL:${OPENRHI_ENABLE_VK_SUPPORT}>:
            "${vulkan_headers_SOURCE_DIR}/include"
        >
    PUBLIC
        "Include"
)
target_compile_definitions(OpenRHI
    PRIVATE
        $<$<NOT:$<BOOL:${OPENRHI_STATIC_LIBRARY}>>:
            $<$<BOOL:${WIN32}>:
                "OPENRHI_API=extern \"C\" __declspec(dllexport)"
            >
            $<$<NOT:$<BOOL:${WIN32}>>:
                "OPENRHI_API=extern \"C\" __attribute__((visibility(\"default\")))"
            >
        >
    PUBLIC
        $<$<BOOL:${OPENRHI_ENABLE_AGILITY_SDK_SUPPORT}>:
            OPENRHI_ENABLE_AGILITY_SDK_SUPPORT=1
        >
)
target_link_libraries(OpenRHI
    PRIVATE
        OpenRHI_Shared
        $<$<NOT:$<BOOL:${WIN32}>>:
            ${CMAKE_DL_LIBS}
        >
        $<$<BOOL:${OPENRHI_ENABLE_NONE_SUPPORT}>:
            OpenRHI_NONE
        >
        $<$<BOOL:${OPENRHI_ENABLE_D3D12_SUPPORT}>:
            OpenRHI_D3D12
        >
        $<$<BOOL:${OPENRHI_ENABLE_VK_SUPPORT}>:
            OpenRHI_VK
        >
        $<$<BOOL:${OPENRHI_ENABLE_METAL_SUPPORT}>:
            OpenRHI_Metal
        >
        $<$<BOOL:${OPENRHI_ENABLE_VALIDATION_SUPPORT}>:
            OpenRHI_Validation
        >
)
set_target_properties(OpenRHI
    PROPERTIES
        FOLDER "OpenRHI"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

message("OpenRHI: output path '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}'")

# Carry the dependency-provided license files with distributable binaries
foreach(DEPENDENCY IN LISTS DEPS)
    set(DEPENDENCY_SOURCE "${${DEPENDENCY}_SOURCE_DIR}")
    if(DEPENDENCY_SOURCE)
        file(GLOB DEPENDENCY_LICENSES LIST_DIRECTORIES false
            "${DEPENDENCY_SOURCE}/*LICENSE*" "${DEPENDENCY_SOURCE}/*License*"
            "${DEPENDENCY_SOURCE}/*license*" "${DEPENDENCY_SOURCE}/*COPYING*")
        foreach(DEPENDENCY_LICENSE IN LISTS DEPENDENCY_LICENSES)
            add_custom_command(TARGET OpenRHI POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:OpenRHI>/ThirdPartyLicenses/${DEPENDENCY}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DEPENDENCY_LICENSE}" "$<TARGET_FILE_DIR:OpenRHI>/ThirdPartyLicenses/${DEPENDENCY}/"
                VERBATIM)
        endforeach()
    endif()
endforeach()

# Copy to the output folder
if(OPENRHI_ENABLE_ANF_SDK)
    add_custom_command(TARGET OpenRHI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENRHI_ANF_SDK_ROOT}/lib/arm64-v8a/libanf.so" "$<TARGET_FILE_DIR:OpenRHI>/"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:OpenRHI>/ThirdPartyLicenses/anf"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENRHI_ANF_SDK_ROOT}/LICENSE.txt" "${OPENRHI_ANF_SDK_ROOT}/LICENSE-BSD-3-Clause.txt" "$<TARGET_FILE_DIR:OpenRHI>/ThirdPartyLicenses/anf/"
        VERBATIM)
endif()

if(OPENRHI_ENABLE_AMDAGS)
    find_file(AMD_AGS_DLL
        NAMES
            amd_ags_${BIN_ARCHITECTURE}.dll
        PATHS "${amdags_SOURCE_DIR}/ags_lib/lib"
    )
    add_custom_command(TARGET OpenRHI
        POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${AMD_AGS_DLL}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM
    )
endif()

if(OPENRHI_ENABLE_AGILITY_SDK_SUPPORT)
    add_custom_command(TARGET OpenRHI
        POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${agilitysdk_SOURCE_DIR}/build/native/bin/${BIN_ARCHITECTURE}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${OPENRHI_AGILITY_SDK_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${agilitysdk_SOURCE_DIR}/LICENSE.txt" "${agilitysdk_SOURCE_DIR}/LICENSE-CODE.txt" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${OPENRHI_AGILITY_SDK_DIR}"
        VERBATIM
    )
endif()


if(OPENRHI_ENABLE_NGX_SDK)
    set(NGX_LIB_DIR $<IF:$<CONFIG:Debug>,dev,rel>)

    if(WIN32)
        set(DLSS_SR_DLL "${ngx_SOURCE_DIR}/lib/${NGX_WINDOWS_DIR}/${NGX_LIB_DIR}/nvngx_dlss.dll")
        set(DLSS_RR_DLL "${ngx_SOURCE_DIR}/lib/${NGX_WINDOWS_DIR}/${NGX_LIB_DIR}/nvngx_dlssd.dll")
    else()
        set(DLSS_SR_DLL "${ngx_SOURCE_DIR}/lib/Linux_x86_64/${NGX_LIB_DIR}/libnvidia-ngx-dlss.so.${NGX_VERSION}")
        set(DLSS_RR_DLL "${ngx_SOURCE_DIR}/lib/Linux_x86_64/${NGX_LIB_DIR}/libnvidia-ngx-dlssd.so.${NGX_VERSION}")
    endif()

    add_custom_command(TARGET OpenRHI
        POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLSS_SR_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLSS_RR_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM
    )
endif()

if(OPENRHI_ENABLE_FFX_SDK)
    if(OPENRHI_ENABLE_D3D12_SUPPORT)
        find_file(FSR_DX12_LOADER_DLL
            NAMES
                amd_fidelityfx_loader_dx12.dll
            PATHS
                "${fsr_SOURCE_DIR}/Kits/FidelityFX/signedbin"
            NO_DEFAULT_PATH
            NO_CACHE
            REQUIRED
        )
        find_file(FSR_DX12_UPSCALER_DLL
            NAMES
                amd_fidelityfx_upscaler_dx12.dll
            PATHS
                "${fsr_SOURCE_DIR}/Kits/FidelityFX/signedbin"
            NO_DEFAULT_PATH
            NO_CACHE
            REQUIRED
        )
        add_custom_command(TARGET OpenRHI
            POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FSR_DX12_LOADER_DLL} ${FSR_DX12_UPSCALER_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                VERBATIM
        )
    endif()

endif()

if(OPENRHI_ENABLE_XESS_SDK)
    if(OPENRHI_ENABLE_D3D12_SUPPORT)
        find_file(XESS_DX12_DLL
            NAMES
                libxess.dll
            PATHS
                "${xess_SOURCE_DIR}/bin"
        )
        add_custom_command(TARGET OpenRHI
            POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${XESS_DX12_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()

if(OPENRHI_ENABLE_FFX_SDK AND OPENRHI_ENABLE_D3D12_SUPPORT)
    foreach(effect IN ITEMS denoiser radiancecache)
        set(runtime "${fsr_SOURCE_DIR}/Kits/FidelityFX/signedbin/amd_fidelityfx_${effect}_dx12.dll")
        if(NOT EXISTS "${runtime}")
            message(FATAL_ERROR "Missing FidelityFX ${effect} runtime: ${runtime}")
        endif()
        add_custom_command(TARGET OpenRHI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${runtime}" "$<TARGET_FILE_DIR:OpenRHI>" VERBATIM)
    endforeach()
endif()
if(OPENRHI_ENABLE_NRC_SDK)
    target_link_options(OpenRHI_Shared INTERFACE /DELAYLOAD:NRC_D3D12.dll)
    target_link_libraries(OpenRHI_Shared PRIVATE delayimp)
    foreach(name IN ITEMS NRC_D3D12.dll cudart64_13.dll nvrtc64_130_0.dll nvrtc-builtins64_134.dll)
        set(runtime "${nrc_SOURCE_DIR}/Bin/${OPENRHI_NRC_ARCH}/${name}")
        if(NOT EXISTS "${runtime}")
            message(FATAL_ERROR "Missing pinned NRC runtime: ${runtime}")
        endif()
        file(READ "${runtime}" signature LIMIT 40)
        if(signature MATCHES "version https://git-lfs")
            message(FATAL_ERROR "NRC runtime is a Git LFS pointer; populate the unmodified pinned SDK with Git LFS: ${runtime}")
        endif()
        add_custom_command(TARGET OpenRHI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${runtime}" "$<TARGET_FILE_DIR:OpenRHI>" VERBATIM)
    endforeach()
endif()

# Slang is the sole shader compilation front end
include(CMake/OpenRHISlang.cmake)
if(OPENRHI_ENABLE_IMGUI_EXTENSION OR OPENRHI_ENABLE_NIS_SDK)
    set(OPENRHI_SHADER_HEADERS)
    if(OPENRHI_ENABLE_IMGUI_EXTENSION)
        foreach(stage IN ITEMS vs fs)
            if(stage STREQUAL "vs")
                set(entry ImguiVS)
                set(kind vertex)
            else()
                set(entry ImguiFS)
                set(kind fragment)
            endif()
            openrhi_compile_shader(NAME Imgui_${stage} SOURCE Shaders/Imgui.slang ENTRY ${entry} STAGE ${kind}
                INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/Include" HEADER header)
            list(APPEND OPENRHI_SHADER_HEADERS "${header}")
        endforeach()
    endif()
    if(OPENRHI_ENABLE_NIS_SDK)
        set(nis_includes "")
        set(nis_table "static const ShaderDesc g_NisShaders[] = {\n")
        foreach(half RANGE 0 1)
            foreach(hdr RANGE 0 1)
                foreach(format RANGE 0 3)
                    if(NOT OPENRHI_ENABLE_VK_SUPPORT AND format GREATER 0)
                        set(name NIS_${half}_${hdr}_0)
                    else()
                        set(name NIS_${half}_${hdr}_${format})
                        openrhi_compile_shader(NAME ${name} SOURCE Shaders/NIS.slang ENTRY NisMain STAGE compute
                            INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/Include" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
                            DEFINES NIS_FP16=${half} NIS_HDR_MODE=${hdr} NIS_OUTPUT_FORMAT=${format} NIS_THREAD_GROUP_SIZE=256 HEADER header)
                        list(APPEND OPENRHI_SHADER_HEADERS "${header}")
                        string(APPEND nis_includes "#include \"${name}.h\"\n")
                    endif()
                    string(APPEND nis_table "    g_${name}_desc,\n")
                endforeach()
            endforeach()
        endforeach()
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/NisShaders.h" "${nis_includes}${nis_table}};\n")
    endif()
    add_custom_target(OpenRHI_Shaders DEPENDS ${OPENRHI_SHADER_HEADERS})
    add_dependencies(OpenRHI_Shared OpenRHI_Shaders)
    target_include_directories(OpenRHI_Shared PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
endif()

# OpenRHI qualification
option(OPENRHI_BUILD_TESTS "Build OpenRHI backend integration tests" OFF)
if(OPENRHI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
