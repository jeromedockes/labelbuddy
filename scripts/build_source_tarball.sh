#! /bin/bash

TARGET_DIR="$(pwd)"
REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. >/dev/null 2>&1 && pwd )"
BUILD_DIR="$(mktemp -d)"

cleanup () {
    rm -rf "$BUILD_DIR"
}
trap 'cleanup' EXIT

cd "$BUILD_DIR"

cmake -DCMAKE_BUILD_TYPE=Release "$REPO_DIR"
make package_source

cp labelbuddy-*-Source.tar.gz* "$TARGET_DIR"
