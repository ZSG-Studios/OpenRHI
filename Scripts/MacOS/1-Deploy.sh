#!/bin/bash
set -e

cd "$(dirname "${BASH_SOURCE[0]}")/../.."

# Deploy with the native Metal backend.
cmake -S . -B _Build -G "Ninja Multi-Config" "$@"
