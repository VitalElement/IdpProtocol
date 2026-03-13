#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIRMWARE_DIR_DEFAULT="/Volumes/SSD/repos/ILMD.Firmware"
FIRMWARE_DIR="${ILMD_FIRMWARE_DIR:-$FIRMWARE_DIR_DEFAULT}"
BUILD_DIR="${ROOT_DIR}/build-cpp-tests"
OUTPUT="${BUILD_DIR}/IdpProtocol.UnitTests"

if [[ ! -d "${FIRMWARE_DIR}" ]]; then
    echo "ILMD.Firmware repo not found at ${FIRMWARE_DIR}" >&2
    echo "Set ILMD_FIRMWARE_DIR to the correct path and retry." >&2
    exit 1
fi

mkdir -p "${BUILD_DIR}"

CPP_SOURCES=(
    "${ROOT_DIR}/src/cpp/"*.cpp
    "${ROOT_DIR}/Tests/IdpProtocol.UnitTests/"*.cpp
)

FIRMWARE_SOURCES=(
    "${FIRMWARE_DIR}/Host/Dispatcher/AvalonStudio.Testing.Catch/main.cpp"
    "${FIRMWARE_DIR}/Host/AvalonApplication/Application.cpp"
    "${FIRMWARE_DIR}/Host/CommonHal/Guid.cpp"
    "${FIRMWARE_DIR}/Host/CommonHal/Trace.cpp"
    "${FIRMWARE_DIR}/Host/Dispatcher/Dispatcher.cpp"
    "${FIRMWARE_DIR}/Host/Dispatcher/DispatcherTimer.cpp"
    "${FIRMWARE_DIR}/Host/Utils/BitConverter.cpp"
    "${FIRMWARE_DIR}/Host/Utils/Event.cpp"
    "${FIRMWARE_DIR}/Host/Utils/Exception.cpp"
)

INCLUDE_DIRS=(
    "${ROOT_DIR}/src/cpp"
    "${ROOT_DIR}/Tests/IdpProtocol.UnitTests"
    "${FIRMWARE_DIR}/Host/CommonHal"
    "${FIRMWARE_DIR}/Host/AvalonApplication"
    "${FIRMWARE_DIR}/Host/Dispatcher"
    "${FIRMWARE_DIR}/Host/Utils"
    "${FIRMWARE_DIR}/Host/Dispatcher/AvalonStudio.Testing.Catch"
)

INCLUDE_ARGS=()
for include_dir in "${INCLUDE_DIRS[@]}"; do
    INCLUDE_ARGS+=("-I${include_dir}")
done

echo "Building ${OUTPUT}"

c++ \
    -std=c++14 \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    "${INCLUDE_ARGS[@]}" \
    "${FIRMWARE_SOURCES[@]}" \
    "${CPP_SOURCES[@]}" \
    -o "${OUTPUT}"

echo "Built ${OUTPUT}"
