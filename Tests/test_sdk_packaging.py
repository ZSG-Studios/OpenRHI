"""Run the SDK preparation scripts in retained, isolated fixture repositories."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


RUNTIMES = (
    "amd_ags_x64.dll",
    "amd_fidelityfx_loader_dx12.dll",
    "amd_fidelityfx_upscaler_dx12.dll",
    "amd_fidelityfx_denoiser_dx12.dll",
    "amd_fidelityfx_radiancecache_dx12.dll",
    "NRC_D3D12.dll",
    "NRC_Vulkan.dll",
    "cudart64_13.dll",
    "nvrtc64_130_0.dll",
    "nvrtc-builtins64_134.dll",
    "amd_fidelityfx_vk.dll",
    "nvngx_dlss.dll",
    "nvngx_dlssd.dll",
    "libxess.dll",
)


def prepare_fixture(root, output, platform):
    fixture = Path(tempfile.mkdtemp(prefix=f"sdk-{platform}-", dir=output))
    for directory in ("Include", "CMake", "Docs"):
        shutil.copytree(root / directory, fixture / directory)
    for filename in (
        "LICENSE.txt", "THIRD_PARTY_NOTICES.md", "README.md", "openrhi.natvis",
    ):
        shutil.copyfile(root / filename, fixture / filename)
    scripts = fixture / "Scripts" / platform
    scripts.mkdir(parents=True)
    extension = "bat" if platform == "Windows" else "sh"
    script = Path("Scripts") / platform / f"3-PrepareSDK.{extension}"
    shutil.copyfile(root / script, fixture / script)
    if platform == "Linux":
        shutil.copyfile(root / "Scripts/Trash.sh", fixture / "Scripts/Trash.sh")
    return fixture, fixture / script


def write_file(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def check_file(source, packaged):
    if not packaged.is_file() or packaged.read_bytes() != source.read_bytes():
        raise AssertionError(f"Missing or changed packaged file: {packaged}")


def run_windows(root, output, optional):
    fixture, script = prepare_fixture(root, output, "Windows")
    files = []
    for configuration in ("Debug", "Release"):
        source = fixture / "_Bin" / configuration
        names = ["OpenRHI.dll", "OpenRHI.lib"]
        if configuration == "Debug":
            names.append("OpenRHI.pdb")
        if optional:
            names.extend(RUNTIMES)
        names.extend(("AgilitySDK/D3D12Core.dll", "ThirdPartyLicenses/vendor/LICENSE.txt"))
        for name in names:
            path = source / name
            write_file(path, f"{configuration}/{name}".encode())
            files.append((path, fixture / "_OpenRHI_SDK/Lib" / configuration / name))
    result = subprocess.run([os.environ.get("COMSPEC", "cmd.exe"), "/d", "/c", str(script)],
                            cwd=output, capture_output=True, text=True)
    (fixture / "prepare.log").write_text(result.stdout + result.stderr)
    result.check_returncode()
    for guide in (fixture / "Docs").glob("*.md"):
        check_file(guide, fixture / "_OpenRHI_SDK/Docs" / guide.name)
    for source, packaged in files:
        check_file(source, packaged)
    print(f"Windows SDK optional={optional}: PASS ({fixture})")


def run_linux(root, output, bash, optional):
    fixture, script = prepare_fixture(root, output, "Linux")
    names = ["libOpenRHI.so", "ThirdPartyLicenses/vendor/LICENSE.txt"]
    if optional:
        names.extend(("libnvidia-ngx-dlss.so.310.9.1", "libnvidia-ngx-dlssd.so.310.9.1", "libanf.so",
                      "ThirdPartyLicenses/anf/LICENSE.txt", "ThirdPartyLicenses/anf/LICENSE-BSD-3-Clause.txt"))
    for name in names:
        write_file(fixture / "_Bin" / name, name.encode())
    if optional and os.name != "nt":
        (fixture / "_Bin/libnvidia-ngx-dlss.so").symlink_to("libnvidia-ngx-dlss.so.310.9.1")
        names.append("libnvidia-ngx-dlss.so")
    result = subprocess.run([bash, script.as_posix()], cwd=output,
                            capture_output=True, text=True)
    (fixture / "prepare.log").write_text(result.stdout + result.stderr)
    result.check_returncode()
    for guide in (fixture / "Docs").glob("*.md"):
        check_file(guide, fixture / "_OpenRHI_SDK/Docs" / guide.name)
    for name in names:
        check_file(fixture / "_Bin" / name, fixture / "_OpenRHI_SDK/Lib" / name)
    for name in names:
        if (fixture / "_OpenRHI_SDK/Lib" / name).is_symlink():
            raise AssertionError(f"Packaged runtime points outside the SDK: {name}")
    print(f"Linux SDK optional={optional}: PASS ({fixture})")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--bash")
    args = parser.parse_args()
    root = args.root.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    for optional in (False, True):
        if os.name == "nt":
            run_windows(root, output, optional)
        if os.name != "nt" or args.bash:
            run_linux(root, output, args.bash or "bash", optional)


if __name__ == "__main__":
    main()
