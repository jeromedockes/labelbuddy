#! /bin/bash

TARGET_DIR="$(pwd)"
REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. >/dev/null 2>&1 && pwd )"
BUILD_DIR="$(mktemp -d)"

cleanup () {
    rm -rf "$BUILD_DIR"
}
trap 'cleanup' EXIT

cd "$BUILD_DIR"
echo "Building AppImage in $(pwd)"

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr "$REPO_DIR"
make DESTDIR=AppDir install

wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod u+x linuxdeploy-*.AppImage


./linuxdeploy-x86_64.AppImage --appdir AppDir
./linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt --output appimage

APPIMAGE=$(find -maxdepth 1 -name 'labelbuddy*.AppImage' -printf '%f')
LABELBUDDY_VERSION=$(cat "$REPO_DIR/data/VERSION.txt")
APPIMAGE_WITH_VERSION=$(echo "$APPIMAGE" | sed "s/^labelbuddy/labelbuddy-$LABELBUDDY_VERSION/")
mv "$APPIMAGE" "$APPIMAGE_WITH_VERSION"
chmod u+x "$APPIMAGE_WITH_VERSION"
md5sum "$APPIMAGE_WITH_VERSION" > "$APPIMAGE_WITH_VERSION.md5"
cp $APPIMAGE_WITH_VERSION* "$TARGET_DIR"
cp -r AppDir "$TARGET_DIR/AppDir"

echo "AppImage in $TARGET_DIR"
