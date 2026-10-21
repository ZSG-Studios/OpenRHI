#!/bin/bash
set -e
cd "$(dirname "${BASH_SOURCE[0]}")/../.."
bash Scripts/Trash.sh Build Bin .xmake _Bin _Build _Shaders _Tmp _OpenRHI_SDK
