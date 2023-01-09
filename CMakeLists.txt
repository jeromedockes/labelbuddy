cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(READ "${CMAKE_SOURCE_DIR}/data/VERSION.txt" LABELBUDDY_VERSION)
string(REGEX REPLACE "\n" "" LABELBUDDY_VERSION "${LABELBUDDY_VERSION}")

project(labelbuddy VERSION "${LABELBUDDY_VERSION}" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  if (MSVC)
    add_compile_options(/W4 /WX)
  else()
    add_compile_options(-Wall -Wextra -Wconversion -pedantic -Werror)
  endif()
else()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets Sql REQUIRED)

add_executable(labelbuddy
  src/main.cpp
  src/doc_list.cpp
  src/doc_list_model.cpp
  src/label_list.cpp
  src/label_list_model.cpp
  src/dataset_menu.cpp
  src/main_window.cpp
  src/annotator.cpp
  src/searchable_text.cpp
  src/annotations_model.cpp
  src/no_deselect_all_view.cpp
  src/database.cpp
  src/import_export_menu.cpp
  src/utils.cpp
  src/compat.cpp
  src/char_indices.cpp
  resources.qrc
  )

target_link_libraries(labelbuddy Qt5::Widgets Qt5::Sql)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

set(CPACK_PACKAGE_NAME "labelbuddy")
set(CPACK_PACKAGE_VERSION "${LABELBUDDY_VERSION}")

string(REGEX REPLACE
  "([0-9]+)[.][0-9]+[.][0-9]+" "\\1"
  LB_MAJOR_VERSION "${LABELBUDDY_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${LB_MAJOR_VERSION}")

string(REGEX REPLACE
  "[0-9]+[.]([0-9]+)[.][0-9]+" "\\1"
  LB_MINOR_VERSION "${LABELBUDDY_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${LB_MINOR_VERSION}")

string(REGEX REPLACE
  "[0-9]+[.][0-9]+[.]([0-9]+)" "\\1"
  LB_PATCH_VERSION "${LABELBUDDY_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${LB_PATCH_VERSION}")

set(CPACK_PACKAGE_CHECKSUM "MD5")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/jeromedockes/labelbuddy")
if(CMAKE_VERSION VERSION_LESS "3.14.0")
  file(READ "${CMAKE_SOURCE_DIR}/docs/Description" DESCR)
  string(REGEX REPLACE "\n$" "" CPACK_DEBIAN_PACKAGE_DESCRIPTION "${DESCR}")
else()
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GUI tool for annotating documents")
  set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/docs/extended-description")
endif()
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  TODOs.org
  org-attachments/
  compile_commands.json
  [.]git/
  [.]gitignore
  [.]gitattributes
  [.]cache/
  build/
  cmake_build/
  cmake_debug_build/
  cmake_release_build/
  qmake_build/
  GPATH
  GRTAGS
  GTAGS
  test/labelbuddy_tests
  test_build
  test_cli/data/newsgroups/
  test_cli/__pycache__/
  test_cli/[.]mypy_cache/
  [.]mypy_cache/
  [.]pytest_cache/
  test_cli/.*[.]pyc
  doxygen_output/
  docs/modules/ROOT/images/.*[.]png
  moc_.*
  src/moc_.*
  test/moc_.*
  qrc_resources[.]cpp
  qrc_test_resources[.]cpp
  [.]qmake[.]stash
  .*[.]d$
  .*[.]slo
  .*[.]lo
  .*[.]o
  .*[.]obj
  .*[.]gch
  .*[.]pch
  .*[.]so
  .*[.]dylib
  .*[.]dll
  .*[.]mod
  .*[.]smod
  .*[.]lai
  .*[.]la
  .*[.]a$
  .*[.]lib
  .*[.]exe
  .*[.]out
  .*[.]app
  )

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_VERSION "${LABELBUDDY_VERSION}-1")
set(CPACK_DEBIAN_PACKAGE_SECTION "text")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jerome Dockes <jerome@dockes.org>")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(GNUInstallDirs)
install(TARGETS labelbuddy
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

file(WRITE "${CMAKE_BINARY_DIR}/copyright"
  "Copyright (C) 2021 ${CPACK_DEBIAN_PACKAGE_MAINTAINER}

 This package is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This package is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this package; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

 On Debian systems, the complete text of the GNU General
 Public License can be found in `/usr/share/common-licenses/GPL-3'.")

execute_process(COMMAND gzip -9 -c -n ${CMAKE_SOURCE_DIR}/docs/changelog
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  OUTPUT_FILE "${CMAKE_BINARY_DIR}/changelog.Debian.gz")

install(FILES "${CMAKE_BINARY_DIR}/changelog.Debian.gz"
  "${CMAKE_BINARY_DIR}/copyright"
  "${CMAKE_SOURCE_DIR}/docs/documentation.html"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}"
  )

execute_process(COMMAND gzip -9 -c -n ${CMAKE_SOURCE_DIR}/docs/labelbuddy.1
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  OUTPUT_FILE "${CMAKE_BINARY_DIR}/labelbuddy.1.gz")

install(FILES "${CMAKE_BINARY_DIR}/labelbuddy.1.gz"
  DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

install (FILES "${CMAKE_SOURCE_DIR}/data/labelbuddy.desktop"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")

install (FILES "${CMAKE_SOURCE_DIR}/data/icons/labelbuddy.png"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps")

install (FILES "${CMAKE_SOURCE_DIR}/data/icons/labelbuddy.svg"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")

include(CPack)
