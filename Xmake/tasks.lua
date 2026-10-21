local shader_root = path.directory(os.scriptdir())
task("openrhi-shader")
    set_category("plugin")
    set_menu {usage = "xmake openrhi-shader [options]", description = "Compile a pinned Slang shader and reflection metadata", options = {
        {nil, "source", "kv", nil, "Input Slang source"}, {nil, "entry", "kv", nil, "Entry point"},
        {nil, "stage", "kv", "compute", "Shader stage"}, {nil, "target", "kv", nil, "dxil, spirv or metal"},
        {nil, "output", "kv", nil, "Binary output"}, {nil, "reflection", "kv", nil, "Reflection output"},
        {nil, "header", "kv", nil, "C++ metadata header"}, {nil, "symbol", "kv", nil, "Header symbol"},
        {nil, "name", "kv", nil, "Header symbol alias"}, {nil, "include", "kv", nil, "Include directories separated by |"},
        {nil, "define", "kv", nil, "Definitions separated by |"}, {nil, "slang-root", "kv", nil, "Pinned Slang root"},
        {nil, "dxil-root", "kv", nil, "Pinned downstream DXIL root"},
        {nil, "task-payload-size", "kv", "0", "Declared task payload bytes"}, {nil, "apple-sdk", "kv", "macosx", "Apple compiler SDK"}
    }}
    on_run(function ()
        import("core.base.option")
        import("openrhi_slang", {rootdir = path.join(shader_root, "Xmake")})
        local options = {root = shader_root}
        for _, key in ipairs({"source", "entry", "stage", "target", "output", "reflection", "header", "symbol", "slang_root", "dxil_root", "task_payload_size", "apple_sdk"}) do
            options[key] = option.get(key:gsub("_", "-"))
        end
        options.symbol = options.symbol or option.get("name")
        options.includes = option.get("include") and option.get("include"):split("|", {plain = true}) or {}
        options.defines = option.get("define") and option.get("define"):split("|", {plain = true}) or {}
        assert(options.source and options.target and options.output, "source, target and output are required")
        openrhi_slang.compile(options)
    end)
task_end()
