#! /bin/bash

if [[ ! $(basename $1) =~ ^labelbuddy-.*.tar.gz$ ]]; then
    echo "Specify source tarball path"
    exit 1
fi

TARGET_DIR="$(pwd)"
BUILD_DIR="$(mktemp -d)"

cleanup () {
    rm -rf "$BUILD_DIR"
}
trap 'cleanup' EXIT

cp "$1" "$BUILD_DIR"
cd "$BUILD_DIR"
archive=$(find -name 'labelbuddy*.tar.gz')
tar xzf "$archive"
mkdir build
cd build
srcdir=$(find .. -maxdepth 1 -type d -name 'labelbuddy*')
cmake -DCMAKE_BUILD_TYPE=Release "$srcdir" && cmake --build .
cpack

codename=$(lsb_release -cs)
old_package_name=$(find -maxdepth 1 -name '*.deb' -printf '%f')
package_name=$(echo "$old_package_name" | sed -r "s/(labelbuddy_[0-9]\.[0-9]\.[0-9]-1)(_.*\.deb)/\1~${codename}1\2/")
mv "$old_package_name" "$package_name"
rm *.md5
md5sum "$package_name" > "$package_name.md5"
cp labelbuddy*.deb* "$TARGET_DIR"
