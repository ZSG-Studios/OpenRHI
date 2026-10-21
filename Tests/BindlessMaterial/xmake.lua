set_project("OpenRHI-BindlessMaterial")
set_xmakever("3.1.2")
set_languages("cxx17")
add_rules("mode.debug", "mode.release")
option("sdk_root") set_default(path.join(os.projectdir(), "../../_OpenRHI_SDK")) option_end()
option("fixture_output") set_default(path.join(os.projectdir(), "../../Build/BindlessMaterial")) option_end()
local sdk = path.absolute(get_config("sdk_root"))
local out = path.absolute(get_config("fixture_output"))
local mode = is_mode("debug") and "Debug" or "Release"
local here = os.projectdir()
target("OpenRHIBindlessMaterialShaders")
    set_kind("phony")
    on_build(function ()
        for _, variant in ipairs({{"Heap", "0"}, {"Conventional", "1"}}) do
            local name = "BindlessMaterial" .. variant[1]
            os.vrunv(os.getenv("PYTHON") or "python", {path.join(sdk, "Scripts/Build/shader.py"),
                "--source", path.join(here, "Material.slang"), "--entry", "materialMain", "--stage", "compute", "--target", "dxil",
                "--output", path.join(out, name .. ".dxil"), "--define", "OPENRHI_MATERIAL_CONVENTIONAL=" .. variant[2]})
        end
    end)
target_end()
target("OpenRHIBindlessMaterial")
    set_kind("binary")
    add_deps("OpenRHIBindlessMaterialShaders")
    add_files("Main.cpp")
    add_includedirs(path.join(sdk, "Include"))
    add_linkdirs(path.join(sdk, "Lib", mode))
    add_links("OpenRHI")
    set_targetdir(path.join(out, mode))
    set_objectdir(path.join(out, "Objects", mode))
    if is_plat("windows") then set_runtimes("MD"); add_cxxflags("/W4", "/WX", "/utf-8", "/Zc:__cplusplus") end
    after_build(function(target)
        for _, runtime in ipairs(os.files(path.join(sdk, "Lib", mode, "*.dll"))) do os.cp(runtime, target:targetdir()) end
        for _, directory in ipairs({"AgilitySDK", "ThirdPartyLicenses"}) do
            local source = path.join(sdk, "Lib", mode, directory)
            if os.isdir(source) then os.cp(source, path.join(target:targetdir(), directory)) end
        end
    end)
    add_tests("reference", {runargs = {"--cpu-only"}})
target_end()
