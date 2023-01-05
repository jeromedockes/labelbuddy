#! /bin/bash

if [[ ! -z "$(ls -A $(pwd))" ]]; then
    echo "Please run from an empty directory!"
    exit 1
fi

if [[ -z $1 || ! $(basename $1) =~ ^labelbuddy-[0-9]\.[0-9]\.[0-9]-Source.tar.gz$ ]]; then
    cat <<EOF
Please pecify source tarball path:

$(basename $0) /path/to/labelbuddy-x.x.x-Source.tar.gz
EOF
    exit 1
fi

TARBALL_NAME="$(basename $1)"
LABELBUDDY_VERSION=$(echo "$TARBALL_NAME" | sed 's/labelbuddy-\([0-9]\.[0-9]\.[0-9]\)-Source\.tar\.gz/\1/')
echo "Building deb package for labelbuddy ${LABELBUDDY_VERSION}"

BUILD_DIR="$(pwd)"
REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. >/dev/null 2>&1 && pwd )"

cp "$1" "$BUILD_DIR"
cd "$BUILD_DIR"

tar xzf "$TARBALL_NAME"
SRC_DIR="labelbuddy-$LABELBUDDY_VERSION"
mv "labelbuddy-$LABELBUDDY_VERSION-Source" "$SRC_DIR"
cd "$SRC_DIR"

export DEBEMAIL="jerome@dockes.org"
export DEBFULLNAME="Jerome Dockes"
bzr whoami "Jerome Dockes <jerome@dockes.org>"

dh_make -s --createorig -y

cd debian
rm -f *.ex *.EX README.source README.Debian labelbuddy-docs.docs
cat <<'EOF' > control
Source: labelbuddy
Section: text
Priority: optional
Maintainer: Jerome Dockes <jerome@dockes.org>
Build-Depends: debhelper (>= 9.0.0), cmake (>= 3.1.0), qtbase5-dev
Standards-Version: 4.1.2
Homepage: https://jeromedockes.github.io/labelbuddy/
Vcs-Git: https://github.com/jeromedockes/labelbuddy.git

Package: labelbuddy
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}
Description: GUI tool for annotating documents
 This is an application for annotating parts of documents with labels.
 labelbuddy can be used for Named Entity Recognition,
 sentiment analysis and document classification, etc.
 It depends on Qt5.
EOF

DISTRIB_CODENAME="$(lsb_release -cs)"

if [[ $DISTRIB_CODENAME =~ ^(buster|stretch)$ ]]; then
    sed -i 's/qtbase5-dev/qt5-default/' control
fi

cat <<'EOF' > copyright
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: labelbuddy
Upstream-Contact: Jerome Dockes <jerome@dockes.org>
Source: https://jeromedockes.github.io/labelbuddy/

Files: *
Copyright: 2021 Jerome Dockes <jerome@dockes.org>
License: GPL-3+

License: GPL-3+
 This package is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 .
 This package is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 You should have received a copy of the GNU General Public License
 along with this package; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 .
 On Debian systems, the complete text of the GNU General
 Public License can be found in `/usr/share/common-licenses/GPL-3'.
EOF

cat <<'EOF' > rules
#!/usr/bin/make -f

%:
	dh $@


EOF

echo "10" > compat

cd ..
bzr init
bzr add debian
bzr commit -m "packaging"
sed "s/^labelbuddy (\(.*\)) unstable;\(.*\)$/labelbuddy (\1-1~${DISTRIB_CODENAME}1) $DISTRIB_CODENAME;\2/" docs/changelog > debian/changelog
bzr builddeb -- -us -uc

cd $BUILD_DIR
PACKAGE_FILE_NAME=$(find . -maxdepth 1 -name 'labelbuddy_*.deb' -printf '%f')
md5sum "$PACKAGE_FILE_NAME" > "$PACKAGE_FILE_NAME.md5"

cat <<EOF


Package built in $BUILD_DIR/$PACKAGE_FILE_NAME
To sign changes and dsc file run:

cd $SRC_DIR
bzr builddeb -S
EOF
