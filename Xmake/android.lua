local r = os.projectdir()
local deps = os.getenv("OPENRHI_DEPS_ROOT") or path.join(r, "Build/Dependencies")
local shader_output = path.join(r, "Build/AndroidLab/Shaders")
target("OpenRHINativeLab")
    set_kind("shared")
    set_default(false)
    add_deps("OpenRHI")
    add_files(path.join(r,"Tests/AndroidDeviceLab/NativeTests.cpp"), path.join(r,"Tests/AndroidDeviceLab/AnfDispatch.cpp"), path.join(r,"Tests/PublicFacade.c"))
    for _, source in ipairs({{"DeviceQualification.cpp","OpenRHIQualificationMain"}, {"BackendSmoke.cpp","OpenRHISmokeMain"},
        {"DevicePolicyTests.cpp","OpenRHIPolicyMain"}, {"PublicApiTests.cpp","OpenRHIPublicMain"}, {"AndroidDeviceLab/CompatibilityProbe.cpp","ANFCompatibilityMain"}}) do
        add_files(path.join(r,"Tests",source[1]), {defines = {"main=" .. source[2]}})
    end
    add_includedirs(path.join(r,"Include"), path.join(r,"Source/Shared"), path.join(get_config("anf_root"),"public"),
        path.join(os.getenv("OPENRHI_VULKAN_HEADERS_ROOT") or path.join(deps,"vulkan_headers"),"include"))
    add_defines("OPENRHI_TEST_NONE=1", 'LAB_CONFIGURATION="' .. (is_mode("debug") and "Debug" or "Release") .. '"')
    add_syslinks("android","log","vulkan","dl")
    add_shflags("-Wl,-z,max-page-size=16384", {force = true})
    before_build(function()
        local shaders = {{"DescriptorCompute","Tests/DescriptorCompute.slang","descriptorMain","compute"},
            {"IndirectDraw.vs","Tests/IndirectDraw.slang","vsMain","vertex"}, {"IndirectDraw.ps","Tests/IndirectDraw.slang","psMain","fragment"},
            {"MeshDraw","Tests/MeshDraw.slang","meshMain","mesh"}, {"TaskDraw","Tests/MeshDraw.slang","taskMain","amplification"},
            {"CacheProbe","Tests/AndroidDeviceLab/CacheProbe.slang","cacheMain","compute"}}
        for _, shader in ipairs(shaders) do
            os.vrunv(os.getenv("PYTHON") or "python", {path.join(r,"Scripts/Build/shader.py"),"--source",path.join(r,shader[2]),
                "--entry",shader[3],"--stage",shader[4],"--target","spirv","--include",path.join(r,"Include"),"--output",path.join(shader_output,shader[1]..".spirv")})
        end
    end)
target_end()
