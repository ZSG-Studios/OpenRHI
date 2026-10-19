"""Add native test payloads to the APK created by aapt2."""
import pathlib
import sys
import zipfile

apk, dex, runner, anf, validation = map(pathlib.Path, sys.argv[1:])
with zipfile.ZipFile(apk, "a", compression=zipfile.ZIP_DEFLATED) as package:
    package.write(dex, "classes.dex")
    for library in (runner, anf, validation):
        package.write(library, f"lib/arm64-v8a/{library.name}")
