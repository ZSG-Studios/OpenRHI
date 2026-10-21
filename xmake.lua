set_project("OpenRHI")
set_version("1.0.0")
set_xmakever("3.1.2")
set_languages("c11", "cxx17")
add_rules("mode.debug", "mode.release")
set_objectdir("Build/Objects/$(plat)/$(arch)/$(mode)/$(target)")
set_targetdir("Bin/$(plat)/$(arch)/$(mode)")
includes("Xmake/tasks.lua")

local default_plat = get_config("plat")
if not default_plat or default_plat == "auto" then default_plat = os.host() end
local default_arch = get_config("arch")
if not default_arch or default_arch == "auto" then default_arch = os.arch() end
local function default_is_plat(...)
    for _, value in ipairs({...}) do if default_plat == value then return true end end
    return false
end
local function default_is_arch(...)
    for _, value in ipairs({...}) do if default_arch == value then return true end end
    return false
end
local defaults = {
    static_library = default_is_plat("iphoneos"), debug_names = true, none = false, validation = true,
    imgui = true, rmlui = true, streamer_thread_safe = true, nvtx = not default_is_plat("iphoneos"),
    d3d12 = default_is_plat("windows"), vk = default_is_plat("linux", "android"), metal = default_is_plat("macosx", "iphoneos"),
    nis = false, amdags = default_is_plat("windows") and default_is_arch("x64", "arm64ec"), nvapi = default_is_plat("windows"),
    agility = default_is_plat("windows"), xlib = default_is_plat("linux"), wayland = default_is_plat("linux"),
    ngx = false, ffx = default_is_plat("windows") and default_is_arch("x64", "arm64ec"), xess = false, nrc = false,
    antilag2 = default_is_plat("windows") and default_is_arch("x64", "arm64ec"), frame_generation = default_is_plat("windows") and default_is_arch("x64", "arm64ec"),
    anf = false, metal_cpp_private = true, build_tests = false
}
for name, value in pairs(defaults) do
    option(name)
        set_default(value)
        set_showmenu(true)
        set_description("OpenRHI " .. name:gsub("_", " "))
    option_end()
end
option("apple_sdk") set_default(default_is_plat("iphoneos") and "iphoneos" or "macosx") set_showmenu(true) option_end()
option("anf_root") set_default("") set_showmenu(true) option_end()
option("nvapi_root") set_default("") set_showmenu(true) option_end()

local definitions = {
    static_library = "OPENRHI_STATIC_LIBRARY", debug_names = "OPENRHI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS",
    none = "OPENRHI_ENABLE_NONE_SUPPORT", validation = "OPENRHI_ENABLE_VALIDATION_SUPPORT",
    imgui = "OPENRHI_ENABLE_IMGUI_EXTENSION", rmlui = "OPENRHI_ENABLE_RMLUI_EXTENSION",
    streamer_thread_safe = "OPENRHI_STREAMER_THREAD_SAFE", nvtx = "OPENRHI_ENABLE_NVTX_SUPPORT",
    d3d12 = "OPENRHI_ENABLE_D3D12_SUPPORT", vk = "OPENRHI_ENABLE_VK_SUPPORT", metal = "OPENRHI_ENABLE_METAL_SUPPORT",
    nis = "OPENRHI_ENABLE_NIS_SDK", amdags = "OPENRHI_ENABLE_AMDAGS", nvapi = "OPENRHI_ENABLE_NVAPI",
    agility = "OPENRHI_ENABLE_AGILITY_SDK_SUPPORT", xlib = "OPENRHI_ENABLE_XLIB_SUPPORT", wayland = "OPENRHI_ENABLE_WAYLAND_SUPPORT",
    ngx = "OPENRHI_ENABLE_NGX_SDK", ffx = "OPENRHI_ENABLE_FFX_SDK", xess = "OPENRHI_ENABLE_XESS_SDK",
    nrc = "OPENRHI_ENABLE_NRC_SDK", antilag2 = "OPENRHI_ENABLE_ANTILAG2_SDK", frame_generation = "OPENRHI_ENABLE_FFX_FRAME_GENERATION",
    anf = "OPENRHI_ENABLE_ANF_SDK", metal_cpp_private = "OPENRHI_METAL_CPP_PRIVATE_IMPLEMENTATION"
}
local root = os.projectdir()
local deps = os.getenv("OPENRHI_DEPS_ROOT") or path.join(root, "Build/Dependencies")
local generated = path.join(root, "Build/Generated", get_config("plat") or os.host(), get_config("arch") or os.arch(), get_config("mode") or "release")
local features = {}
for name, macro in pairs(definitions) do
    if has_config(name) then table.insert(features, name) end
end
table.sort(features)
local feature_arg = table.concat(features, ",")

local function base_options()
    add_includedirs("Include", "Source/Shared", generated)
    add_defines("WIN32_LEAN_AND_MEAN", "NOMINMAX", "_CRT_SECURE_NO_WARNINGS")
    if is_plat("windows") then
        set_runtimes("MD")
        add_cxxflags("/W4", "/WX", "/wd4324", "/utf-8", "/Zc:__cplusplus")
    else
        add_cxxflags("-Wextra", "-Werror", "-fvisibility=hidden", "-Wno-missing-field-initializers", "-Wno-nullability-completeness")
        add_cxflags("-fPIC")
    end
end

local function dependency_includes()
    local dirs = {d3d12 = {"d3d12ma/include", "d3d12ma/src"}, agility = {"agilitysdk/build/native/include"},
        nvtx = {"nvtx/c/include"}, vk = {"vulkan_headers/include", "vma/include"}, metal = {"metal_cpp"},
        nvapi = {"nvapi"}, amdags = {"amdags/ags_lib/inc"}, ngx = {"ngx/include"}, antilag2 = {"antilag2"},
        xess = {"xess/inc/xess"}, nrc = {"nrc/Include"}}
    for feature, paths in pairs(dirs) do
        if has_config(feature) then
            for _, directory in ipairs(paths) do add_includedirs(path.join(deps, directory)) end
        end
    end
    if has_config("nvapi") and get_config("nvapi_root") ~= "" then add_includedirs(get_config("nvapi_root")) end
    if has_config("vk") then
        if os.getenv("OPENRHI_VULKAN_HEADERS_ROOT") then add_includedirs(path.join(os.getenv("OPENRHI_VULKAN_HEADERS_ROOT"),"include"), {before = true}) end
        if os.getenv("OPENRHI_VMA_ROOT") then add_includedirs(path.join(os.getenv("OPENRHI_VMA_ROOT"),"include"), {before = true}) end
    end
    if has_config("ffx") then
        if has_config("d3d12") then
            for _, item in ipairs({"api", "upscalers", "denoisers", "radiancecache", "framegeneration"}) do
                add_includedirs(path.join(deps, "fsr/Kits/FidelityFX", item, "include"))
            end
        else add_includedirs(path.join(deps, "ffx/ffx-api/include/ffx_api")) end
    end
    if has_config("anf") then add_includedirs(path.join(get_config("anf_root"), "public")) end
end

target("OpenRHI")
    set_kind(has_config("static_library") and "static" or "shared")
    base_options()
    dependency_includes()
    add_includedirs("Include", generated, {public = true})
    if has_config("agility") then add_defines("OPENRHI_ENABLE_AGILITY_SDK_SUPPORT=1", {public = true}) end
    for name, macro in pairs(definitions) do
        if has_config(name) then add_defines(macro .. "=1") end
    end
    if has_config("static_library") then add_defines("OPENRHI_STATIC_LIBRARY=1", {public = true})
    elseif is_plat("windows") then add_defines('OPENRHI_API=extern "C" __declspec(dllexport)')
    else add_defines('OPENRHI_API=extern "C" __attribute__((visibility("default")))') end
    add_files("Source/Creation/Creation.cpp", "Source/Shared/Shared.cpp")
    if has_config("validation") then add_files("Source/Validation/ImplVal.cpp") end
    if has_config("none") then add_files("Source/NONE/ImplNONE.cpp") end
    if has_config("d3d12") then
        add_files("Source/D3D12/ImplD3D12.cpp", "Resources/OpenRHI.rc")
        add_syslinks("d3d12", "dxgi", "dxguid", "user32")
        add_defines("OPENRHI_AGILITY_SDK_VERSION_MAJOR=" .. (has_config("agility") and "619" or "D3D12_SDK_VERSION"))
    end
    if has_config("vk") then
        add_files("Source/VK/ImplVK.cpp")
        if is_plat("android") then add_defines("VK_USE_PLATFORM_ANDROID_KHR"); add_syslinks("android", "log") end
        if has_config("xlib") then add_defines("VK_USE_PLATFORM_XLIB_KHR") end
        if has_config("wayland") then add_defines("VK_USE_PLATFORM_WAYLAND_KHR") end
    end
    if has_config("metal") then
        add_files("Source/Metal/ImplMetal.cpp")
        add_frameworks("Foundation", "Metal", "QuartzCore", "CoreGraphics")
        if get_config("apple_sdk") ~= "iphonesimulator" then add_frameworks("MetalFX") end
        add_cxxflags(is_plat("iphoneos") and (get_config("apple_sdk") == "iphonesimulator" and "-mios-simulator-version-min=26.0" or "-miphoneos-version-min=26.0") or "-mmacosx-version-min=26.0", {force = true})
    end
    if is_plat("linux") then add_syslinks("dl", "pthread") elseif is_plat("android") then add_syslinks("dl") end
    if has_config("nvapi") then
        local source = get_config("nvapi_root") ~= "" and get_config("nvapi_root") or path.join(deps, "nvapi")
        add_linkdirs(path.join(source, is_arch("arm64") and "aarch64" or "amd64"))
        add_links(is_arch("arm64") and "nvapia64" or "nvapi64")
    end
    if has_config("ngx") then
        if is_plat("windows") then
            add_linkdirs(path.join(deps, "ngx/lib", is_arch("arm64") and "Windows_aarch64" or "Windows_x86_64/x64"))
            add_links(is_mode("debug") and "nvsdk_ngx_d_dbg" or "nvsdk_ngx_d")
        else add_linkdirs(path.join(deps, "ngx/lib/Linux_x86_64")); add_links("nvsdk_ngx") end
    end
    if has_config("xess") then add_linkdirs(path.join(deps, "xess/lib")); add_links("libxess") end
    if has_config("nrc") then
        add_linkdirs(path.join(deps, "nrc/Lib", is_arch("arm64") and "arm64" or "x64"))
        add_links(has_config("d3d12") and "NRC_D3D12" or "NRC_Vulkan")
        add_syslinks("delayimp"); add_ldflags("/DELAYLOAD:NRC_D3D12.dll")
    end
    on_load(function (target)
        assert(has_config("d3d12") or has_config("vk") or has_config("metal") or has_config("none"), "Enable a native backend or explicit NONE test backend")
        assert(not has_config("d3d12") or is_plat("windows"), "D3D12 requires Windows")
        assert(not has_config("vk") or is_plat("linux", "android"), "Vulkan requires Linux or Android")
        assert(not has_config("metal") or is_plat("macosx", "iphoneos"), "Metal requires Apple platforms")
        assert(not is_plat("android") or (is_arch("arm64-v8a", "arm64") and tonumber(get_config("ndk_sdkver") or "33") >= 33), "Android requires ARM64 API 33+")
    end)
    before_build(function (target)
        os.vrunv(os.getenv("PYTHON") or "python", {path.join(root, "Scripts/Build/support.py"), "prepare", "--features", feature_arg,
            "--generated", generated, "--platform", target:plat(), "--arch", target:arch(), "--mode", get_config("mode"), "--anf-root", get_config("anf_root") or "", "--apple-sdk", get_config("apple_sdk") or "macosx"})
    end)
    after_build(function (target)
        os.vrunv(os.getenv("PYTHON") or "python", {path.join(root, "Scripts/Build/support.py"), "runtime", "--features", feature_arg,
            "--output", target:targetdir(), "--platform", target:plat(), "--arch", target:arch(), "--mode", get_config("mode"), "--anf-root", get_config("anf_root") or "", "--apple-sdk", get_config("apple_sdk") or "macosx"})
    end)
target_end()

if has_config("build_tests") then
    includes("Xmake/tests.lua")
end

task("openrhi-sdk")
    set_category("plugin")
    set_menu {usage = "xmake openrhi-sdk [options]", description = "Package the built OpenRHI SDK", options = {
        {nil, "output", "kv", "_OpenRHI_SDK", "SDK output directory"}, {nil, "with-compiler", "k", nil, "Package pinned shader compiler distributions"}}}
    on_run(function ()
        import("core.base.option")
        local args = {path.join(root, "Scripts/Build/package.py"), "--output", path.absolute(option.get("output"))}
        if option.get("with-compiler") then table.insert(args, "--with-compiler") end
        os.vrunv(os.getenv("PYTHON") or "python", args)
    end)
task_end()

task("sdk")
    set_category("plugin")
    set_menu {usage = "xmake sdk [options]", description = "Package the built OpenRHI SDK", options = {
        {nil, "output", "kv", "_OpenRHI_SDK", "SDK output directory"}, {nil, "with-compiler", "k", nil, "Package pinned shader compiler distributions"}}}
    on_run(function ()
        import("core.base.option")
        local args = {path.join(root, "Scripts/Build/package.py"), "--output", path.absolute(option.get("output"))}
        if option.get("with-compiler") then table.insert(args, "--with-compiler") end
        os.vrunv(os.getenv("PYTHON") or "python", args)
    end)
task_end()

 task("openrhi-check")
    set_category("plugin")
    set_menu {usage = "xmake openrhi-check", description = "Audit the owned native source boundary", options = {}}
    on_run(function () os.vrunv(os.getenv("PYTHON") or "python", {path.join(root, "Scripts/Build/boundary.py")}) end)
 task_end()

if is_plat("android") then includes("Xmake/android.lua") end

task("openrhi-build-tests")
    set_category("plugin")
    set_menu {usage="xmake openrhi-build-tests", description="Run source, acquisition and SDK packaging contracts", options={}}
    on_run(function()
        local python = os.getenv("PYTHON") or "python"
        os.vrunv(python,{path.join(root,"Scripts/Build/boundary.py")})
        os.vrunv(python,{path.join(root,"Tests/test_xmake_build.py")})
        os.vrunv(python,{path.join(root,"Tests/test_sdk_packaging.py"),"--root",root,"--output",path.join(root,"Build/SDKPackaging")})
        os.vrunv(python,{path.join(root,"Tests/test_qualification.py")})
    end)
task_end()
