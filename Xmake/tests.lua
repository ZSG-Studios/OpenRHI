local test_root = os.projectdir()
local generated = path.join(test_root, "Build/Generated", get_config("plat"), get_config("arch"), get_config("mode"))
local tests = {
    {"OpenRHIPublicAPI", "PublicApiTests.cpp"}, {"OpenRHIDevicePolicy", "DevicePolicyTests.cpp"},
    {"OpenRHITextureViewContract", "TextureViewContract.cpp"}, {"OpenRHIAdapterSelection", "AdapterSelection.cpp"},
    {"OpenRHISmoke", "BackendSmoke.cpp"}, {"OpenRHIUpstreamRegression", "UpstreamRegression.cpp"},
    {"OpenRHIQualification", "DeviceQualification.cpp"}
}
if has_config("none") then
    table.insert(tests, {"OpenRHIUIAvailability", "UIAvailability.cpp"})
    table.insert(tests, {"OpenRHIVendorAvailability", "VendorAvailability.cpp"})
end
if is_plat("windows") and has_config("d3d12") then
    table.insert(tests, {"OpenRHIVendorFeatures", "VendorFeatures.cpp"})
    table.insert(tests, {"OpenRHIAMDFrameGeneration", "AMDFrameGeneration.cpp"})
end
for _, entry in ipairs(tests) do
    target(entry[1])
        set_kind("binary")
        set_default(false)
        add_deps("OpenRHI")
        add_files(path.join(test_root, "Tests", entry[2]))
        add_includedirs(path.join(test_root,"Include"), path.join(test_root,"Source/Shared"), generated)
        add_defines("WIN32_LEAN_AND_MEAN", "NOMINMAX", "OPENRHI_TEST_NONE=" .. (has_config("none") and "1" or "0"),
                    "OPENRHI_TEST_STATIC_LIBRARY=" .. (has_config("static_library") and "1" or "0"),
                    "OPENRHI_TEST_RMLUI=" .. (has_config("rmlui") and "1" or "0"),
                    "OPENRHI_TEST_FFX=" .. ((has_config("ffx") and has_config("d3d12")) and "1" or "0"),
                    "OPENRHI_TEST_NRC=" .. (has_config("nrc") and "1" or "0"),
                    "OPENRHI_TEST_ANTILAG=" .. (has_config("antilag2") and "1" or "0"))
        if entry[1] == "OpenRHIPublicAPI" then add_files(path.join(test_root,"Tests/PublicFacade.c")) end
        if is_plat("windows") then set_runtimes("MD"); add_cxxflags("/utf-8", "/Zc:__cplusplus") end
        if entry[1] == "OpenRHIDevicePolicy" or entry[1] == "OpenRHIPublicAPI" or entry[1] == "OpenRHITextureViewContract" or entry[1] == "OpenRHIUIAvailability" or entry[1] == "OpenRHIVendorAvailability" then
            add_tests("cpu", {timeout = 120})
        elseif entry[1] == "OpenRHIAdapterSelection" then
            if has_config("d3d12") or has_config("vk") or has_config("metal") then add_tests("native", {timeout = 120}) end
        elseif entry[1] == "OpenRHISmoke" then
            if has_config("none") then add_tests("NONE", {runargs = {"NONE"}, timeout = 60}) end
            for _, backend in ipairs({"D3D12", "VK", "METAL"}) do
                if has_config(backend:lower()) then add_tests(backend, {runargs = {backend}, timeout = 120}) end
            end
        end
    target_end()
end

-- Public toolkit inputs are compiled as native Xmake targets without invoking vendor builders.
local dependency_root = os.getenv("OPENRHI_DEPS_ROOT") or path.join(test_root,"Build/Dependencies")
if has_config("imgui") then
    target("OpenRHIImguiTest")
        set_kind("static")
        set_default(false)
        add_includedirs(path.join(dependency_root,"imgui"), {public = true})
        add_defines("IMGUI_DISABLE_OBSOLETE_FUNCTIONS", {public = true})
        on_load(function(target)
            os.vrunv(os.getenv("PYTHON") or "python", {path.join(test_root,"Scripts/Build/dependencies.py"),"imgui"})
            for _, name in ipairs({"imgui.cpp","imgui_draw.cpp","imgui_tables.cpp","imgui_widgets.cpp"}) do target:add("files",path.join(dependency_root,"imgui",name)) end
        end)
    target_end()
end
if has_config("rmlui") then
    target("OpenRHIRmlUiCore")
        set_kind("static")
        set_default(false)
        add_includedirs(path.join(dependency_root,"rmlui/Include"), {public = true})
        add_defines("RMLUI_STATIC_LIB", {public = true})
        -- FIXED BY AI: RmlUi debug logging calls the native MessageBoxA entry point
        if is_plat("windows") then set_runtimes("MD"); add_syslinks("user32", {public = true}) end
        on_load(function(target)
            os.vrunv(os.getenv("PYTHON") or "python", {path.join(test_root,"Scripts/Build/dependencies.py"),"rmlui"})
            for _, pattern in ipairs({"Source/Core/*.cpp","Source/Core/Elements/*.cpp","Source/Core/Layout/*.cpp"}) do
                target:add("files",path.join(dependency_root,"rmlui",pattern))
            end
        end)
    target_end()
    target("OpenRHIRmlUiAdapter")
        set_kind("binary") set_default(false)
        add_files(path.join(test_root,"Tests/RmlUiAdapter.cpp"))
        add_deps("OpenRHI","OpenRHIRmlUiCore")
        add_tests("cpu", {timeout = 120})
        if is_plat("windows") then set_runtimes("MD") end
    target_end()
end
if has_config("rmlui") and has_config("imgui") then
    target("OpenRHIUIRender")
        set_kind("binary") set_default(false)
        add_files(path.join(test_root,"Tests/UIRender.cpp")) add_deps("OpenRHI","OpenRHIImguiTest")
        for _, backend in ipairs({"D3D12","VK","METAL"}) do
            if has_config(backend:lower()) then add_tests(backend,{runargs={backend},timeout=120}) end
        end
    target_end()
end
local function mock_contract(name,source,definitions,includes,dependencies)
    target(name)
        set_kind("binary") set_default(false)
        add_files(path.join(test_root,"Tests",source))
        add_includedirs(path.join(test_root,"Include"),path.join(test_root,"Source/Shared"))
        add_defines("WIN32_LEAN_AND_MEAN","NOMINMAX",table.unpack(definitions))
        for _, directory in ipairs(includes) do add_includedirs(directory) end
        if is_plat("windows") then set_runtimes("MD") end
        before_build(function()
            if #dependencies>0 then os.vrunv(os.getenv("PYTHON") or "python",table.join({path.join(test_root,"Scripts/Build/dependencies.py")},dependencies)) end
        end)
        add_tests("cpu",{timeout=120})
    target_end()
end
if has_config("nrc") and has_config("d3d12") then
    mock_contract("OpenRHINeuralRadianceCacheContract","NeuralRadianceCacheContract.cpp",{"OPENRHI_STATIC_LIBRARY=1","OPENRHI_ENABLE_D3D12_SUPPORT=1","OPENRHI_ENABLE_NRC_SDK=1"},
        {path.join(dependency_root,"nrc/Include"),path.join(dependency_root,"agilitysdk/build/native/include")},{"nrc","agilitysdk"})
end
if has_config("ngx") and has_config("d3d12") then
    mock_contract("OpenRHIUpscalerNgxLifecycle","UpscalerNgxLifecycle.cpp",{"OPENRHI_ENABLE_D3D12_SUPPORT=1","OPENRHI_ENABLE_NGX_SDK=1"},
        {path.join(dependency_root,"ngx/include")},{"ngx"})
end
if has_config("nvapi") and has_config("d3d12") then
    mock_contract("OpenRHIReflexContract","ReflexContract.cpp",{"OPENRHI_STATIC_LIBRARY=1"},
        {path.join(dependency_root,"nvapi"),path.join(dependency_root,"vulkan_headers/include")},{"nvapi","vulkan_headers"})
end
if get_config("anf_root") and os.isfile(path.join(get_config("anf_root"),"public/anf.h")) then
    mock_contract("OpenRHIUpscalerAnfContract","UpscalerAnfContract.cpp",{"OPENRHI_STATIC_LIBRARY=1","OPENRHI_ENABLE_VK_SUPPORT=1","OPENRHI_ENABLE_ANF_SDK=1"},
        {path.join(get_config("anf_root"),"public"),path.join(dependency_root,"vulkan_headers/include")},{"vulkan_headers"})
end
if has_config("vk") then
    target("OpenRHIVulkanInstanceCap")
        set_kind("shared") set_default(false) set_basename("vulkan")
        add_files(path.join(test_root,"Tests/VulkanInstanceCap.cpp"))
        add_includedirs(path.join(dependency_root,"vulkan_headers/include")) add_syslinks("dl")
        set_targetdir(path.join(test_root,"Build/VulkanInstanceCap"))
    target_end()
end

local native_backend = has_config("d3d12") and "D3D12" or (has_config("metal") and "METAL" or "VK")
local shader_target = has_config("d3d12") and "dxil" or (has_config("metal") and "metal" or "spirv")
local shader_extension = shader_target == "metal" and "metallib" or shader_target
local shader_dir = path.join(test_root,"Build/Tests/Shaders")
if has_config("d3d12") or has_config("vk") or has_config("metal") then
    target("OpenRHIQualificationShader")
        set_kind("phony") set_default(false)
        on_build(function()
            local arguments = {path.join(test_root,"Scripts/Build/test_shaders.py"),"--output",shader_dir,"--target",shader_target}
            if has_config("rmlui") then table.insert(arguments,"--ui") end
            os.vrunv(os.getenv("PYTHON") or "python",arguments)
        end)
    target_end()
    target("OpenRHIQualification")
        add_deps("OpenRHIQualificationShader")
        add_tests(native_backend..".resources", {timeout=120,runargs={"--api",native_backend,"--iterations","8","--native-validation",
            "--shader",path.join(shader_dir,"DescriptorCompute."..shader_extension),"--vs",path.join(shader_dir,"IndirectDraw.vs."..shader_extension),
            "--ps",path.join(shader_dir,"IndirectDraw.ps."..shader_extension)}})
        add_tests(native_backend..".mesh", {timeout=120,runargs={"--api",native_backend,"--iterations","2","--native-validation","--mesh",
            "--vs",path.join(shader_dir,"MeshDraw."..shader_extension),"--task",path.join(shader_dir,"TaskDraw."..shader_extension),"--ps",path.join(shader_dir,"IndirectDraw.ps."..shader_extension)}})
        if is_plat("windows") then add_syslinks("user32") end
        if has_config("d3d12") then
            add_tests("D3D12.bindless",{timeout=120,runargs={"--api","D3D12","--iterations","1","--native-validation","--heap-shader",path.join(shader_dir,"BindlessCompute.dxil")}})
            add_tests("D3D12.bindless-textures",{timeout=120,runargs={"--api","D3D12","--iterations","1","--native-validation","--heap-texture-shader",path.join(shader_dir,"BindlessTextureSampler.dxil"),"--heap-texture-image-prefix",path.join(test_root,"Logs/BindlessTextureSampler/readback")}})
        end
    target_end()
end
if is_plat("windows") and has_config("d3d12") then
    target("OpenRHIVendorFeatures") add_tests("native",{timeout=600}) target_end()
    target("OpenRHIAMDFrameGeneration") add_syslinks("user32") add_tests("native",{timeout=180}) target_end()
end
if has_config("imgui") and has_config("rmlui") and is_plat("windows") then
    target("OpenRHIUIRender") add_syslinks("user32") target_end()
end
