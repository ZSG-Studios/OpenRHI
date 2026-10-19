#!/bin/bash
set -e

cd "$(dirname "${BASH_SOURCE[0]}")/../.."

SDK=_OpenRHI_SDK

bash Scripts/Trash.sh _OpenRHI_SDK
mkdir -p "${SDK}/Include" "${SDK}/Lib/Debug" "${SDK}/Lib/Release"

cp -R Include/. "${SDK}/Include"
cp LICENSE.txt THIRD_PARTY_NOTICES.md README.md openrhi.natvis "${SDK}"

cp -L _Bin/Debug/libOpenRHI.dylib "${SDK}/Lib/Debug"
cp -L _Bin/Release/libOpenRHI.dylib "${SDK}/Lib/Release"

for CONFIG in Debug Release; do
    if [ -d "_Bin/${CONFIG}/ThirdPartyLicenses" ]; then
        cp -R "_Bin/${CONFIG}/ThirdPartyLicenses" "${SDK}/Lib/${CONFIG}/"
    fi
done

mkdir -p "${SDK}/CMake"
cp CMake/*.cmake "${SDK}/CMake/"
cp -R Docs "${SDK}/"
