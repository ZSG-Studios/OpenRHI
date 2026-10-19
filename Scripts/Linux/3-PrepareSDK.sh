#!/bin/bash
set -e

cd "$(dirname "${BASH_SOURCE[0]}")/../.."

ROOT=$(pwd)
SELF=${ROOT}
SDK=_OpenRHI_SDK

echo ${SDK}: ROOT=${ROOT}, SELF=${SELF}

bash Scripts/Trash.sh _OpenRHI_SDK

mkdir -p "${SDK}/Include/Extensions"
mkdir -p "${SDK}/Lib"

cp -r "${SELF}/Include/." "${SDK}/Include"
cp -r "${SELF}/Include/Extensions/." "${SDK}/Include/Extensions"
cp "${SELF}/LICENSE.txt" "${SELF}/THIRD_PARTY_NOTICES.md" "${SDK}"
cp "${SELF}/README.md" "${SDK}"
cp "${SELF}/openrhi.natvis" "${SDK}"

cp -H "${ROOT}/_Bin/libOpenRHI.so" "${SDK}/Lib"
for RUNTIME in "${ROOT}"/_Bin/libnvidia-ngx-dlss.so* "${ROOT}"/_Bin/libnvidia-ngx-dlssd.so* "${ROOT}"/_Bin/libanf.so; do
    if [ -e "${RUNTIME}" ]; then
        cp -L "${RUNTIME}" "${SDK}/Lib/"
    fi
done
if [ -d "${ROOT}/_Bin/ThirdPartyLicenses" ]; then
    cp -R "${ROOT}/_Bin/ThirdPartyLicenses" "${SDK}/Lib/"
fi

mkdir -p "${SDK}/CMake"
cp CMake/*.cmake "${SDK}/CMake/"
cp -R Docs "${SDK}/"
