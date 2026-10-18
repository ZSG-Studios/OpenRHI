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
cp "${SELF}/README.md" "${SELF}/MIGRATION.md" "${SELF}/QUALIFICATION.md" "${SDK}"
cp "${SELF}/openrhi.natvis" "${SDK}"

cp -H "${ROOT}/_Bin/libOpenRHI.so" "${SDK}/Lib"
if [ -d "${ROOT}/_Bin/ThirdPartyLicenses" ]; then
    cp -R "${ROOT}/_Bin/ThirdPartyLicenses" "${SDK}/Lib/"
fi

mkdir -p "${SDK}/CMake"
cp CMake/*.cmake "${SDK}/CMake/"
cp SHADERS.md "${SDK}/"
