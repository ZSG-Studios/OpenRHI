cmake_minimum_required(VERSION 3.30)
project(OpenRHISlangConsumer LANGUAGES CXX)
include("${SDK}/CMake/OpenRHISlang.cmake")
openrhi_compile_shader(NAME SdkShader SOURCE consumer.slang ENTRY sdkMain STAGE compute HEADER shader_header)
add_custom_target(ConsumerShader DEPENDS "${shader_header}")
add_executable(consumer main.cpp)
add_dependencies(consumer ConsumerShader)
target_compile_features(consumer PRIVATE cxx_std_17)
target_include_directories(consumer PRIVATE "${SDK}/Include" "${CMAKE_CURRENT_BINARY_DIR}")
if(WIN32)
    target_link_libraries(consumer PRIVATE "${SDK}/Lib/Release/OpenRHI.lib")
    add_custom_command(TARGET consumer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SDK}/Lib/Release/OpenRHI.dll" "$<TARGET_FILE_DIR:consumer>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SDK}/Lib/Release/amd_ags_x64.dll" "$<TARGET_FILE_DIR:consumer>"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${SDK}/Lib/Release/AgilitySDK" "$<TARGET_FILE_DIR:consumer>/AgilitySDK")
elseif(APPLE)
    target_link_libraries(consumer PRIVATE "${SDK}/Lib/Release/libOpenRHI.dylib")
endif()
