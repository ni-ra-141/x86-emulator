#!/usr/bin/env bash

set -u

usage() {
    echo "Usage: $0 <program.asm> [--trace]" >&2
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
    usage
    exit 2
fi

source_file=$1
trace_option=""

if [[ $# -eq 2 ]]; then
    if [[ $2 != "--trace" ]]; then
        echo "Unknown option: $2" >&2
        usage
        exit 2
    fi
    trace_option=--trace
fi

if [[ ${source_file##*.} != asm ]]; then
    echo "Assembly input must have a .asm extension: $source_file" >&2
    exit 2
fi

if [[ ! -f $source_file || ! -r $source_file ]]; then
    echo "Assembly source is not a readable file: $source_file" >&2
    exit 2
fi

if ! command -v nasm >/dev/null 2>&1; then
    echo "NASM was not found. Install NASM and ensure 'nasm' is on PATH." >&2
    exit 127
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "$script_dir/.." && pwd)
emulator=""

if [[ -x $repo_root/build/worklet3 ]]; then
    emulator=$repo_root/build/worklet3
elif [[ -x $repo_root/build-cmake/worklet3 ]]; then
    emulator=$repo_root/build-cmake/worklet3
else
    echo "Worklet 3 was not found. Build it with 'make build/worklet3' or CMake." >&2
    exit 127
fi

temp_dir=$(mktemp -d "${TMPDIR:-/tmp}/x86-emulator-asm.XXXXXX") || {
    echo "Unable to create a temporary directory for assembly output." >&2
    exit 1
}
trap 'rm -rf -- "$temp_dir"' EXIT

binary_file=$temp_dir/program.bin
if ! nasm -f bin -o "$binary_file" "$source_file"; then
    echo "Assembly failed: $source_file" >&2
    exit 1
fi

if [[ ! -s $binary_file ]]; then
    echo "Assembly produced no machine code: $source_file" >&2
    exit 1
fi

if [[ -n $trace_option ]]; then
    "$emulator" "$binary_file" "$trace_option"
else
    "$emulator" "$binary_file"
fi
