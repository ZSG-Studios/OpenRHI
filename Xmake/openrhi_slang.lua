-- Pinned Slang compiler adapter shared by OpenRHI and its SDK consumers
function compile(opt)
    local root = opt.root or path.directory(os.scriptdir())
    local args = {path.join(root, "Scripts/Build/shader.py"), "--source", opt.source,
                  "--target", opt.target, "--output", opt.output}
    for _, key in ipairs({"entry", "stage", "reflection", "header", "symbol", "slang_root", "dxil_root", "task_payload_size", "apple_sdk"}) do
        if opt[key] then table.join2(args, {"--" .. key:gsub("_", "-"), tostring(opt[key])}) end
    end
    for _, include in ipairs(opt.includes or {}) do table.join2(args, {"--include", include}) end
    for _, define in ipairs(opt.defines or {}) do table.join2(args, {"--define", define}) end
    os.vrunv(os.getenv("PYTHON") or "python", args)
end
