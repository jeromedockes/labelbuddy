#! /bin/bash

TARGET_DIR="$(pwd)"
REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. >/dev/null 2>&1 && pwd )"
BUILD_DIR="$(mktemp -d)"

cleanup () {
    rm -rf "$BUILD_DIR"
}
trap 'cleanup' EXIT INT TERM

cd "$BUILD_DIR"
echo "Building AppImage in $(pwd)"

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr "$REPO_DIR"
make DESTDIR=AppDir install

wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod u+x linuxdeploy-*.AppImage


./linuxdeploy-x86_64.AppImage --appdir AppDir
./linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt --output appimage

chmod u+x labelbuddy*.AppImage
cp labelbuddy*.AppImage "$TARGET_DIR"

echo "AppImage in $TARGET_DIR"
