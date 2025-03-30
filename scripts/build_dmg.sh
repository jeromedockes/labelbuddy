#! /bin/bash

set -e

TARGET_DIR="$(pwd)"
REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. >/dev/null 2>&1 && pwd )"
BUILD_DIR="$(mktemp -d)"

cleanup () {
    rm -rf "$BUILD_DIR"
}
trap 'cleanup' EXIT

cd "$BUILD_DIR"
echo "Building .dmg in $(pwd)"

qmake "$REPO_DIR/labelbuddy.pro" "CONFIG += app_bundle" -config release
make
macdeployqt labelbuddy.app -dmg

DMG_NAME="labelbuddy-$(cat $REPO_DIR/data/VERSION.txt).dmg"
mv labelbuddy.dmg "$DMG_NAME"
md5 -r "$DMG_NAME" > "$DMG_NAME.md5"

cp $DMG_NAME* "$TARGET_DIR"

echo "Package created in $TARGET_DIR/$DMG_NAME"
