cmake_minimum_required(VERSION 3.1.0)

project(labelbuddy VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
  resources.qrc
  )

target_link_libraries(labelbuddy Qt5::Widgets Qt5::Sql)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

set(CPACK_PACKAGE_NAME "labelbuddy")
set(CPACK_PACKAGE_VERSION "0.0.1")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 1)
set(CPACK_PACKAGE_CHECKSUM "MD5")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "gui tool for annotating documents")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/docs/short-readme.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  [.]git/
  cmake_build/
  qmake_build/
  Makefile
  test/labelbuddy_tests
  test_build
  docs/README[.]rst
  moc_.*
  src/moc_.*
  test/moc_.*
  qrc_resources[.]cpp
  qrc_test_resources[.]cpp
  [.]qmake[.]stash
  .*[.]d
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
  .*[.]a
  .*[.]lib
  .*[.]exe
  .*[.]out
  .*[.]app
  )

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_VERSION "0.0.1-1")
set(CPACK_DEBIAN_PACKAGE_SECTION "text")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jerome Dockes <jerome@dockes.org>")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(GNUInstallDirs)
install(TARGETS labelbuddy
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

file(WRITE "${CMAKE_BINARY_DIR}/copyright"
  "Copyright (C) 2010-2013 ${CPACK_DEBIAN_PACKAGE_MAINTAINER}

   This software may be licensed under the terms of the
   GNU Lesser General Public License Version 3 (the ``LGPL''),
   or (at your option) any later version.

   Software distributed under the License is distributed
   on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
   express or implied. See the LGPL for the specific language
   governing rights and limitations.

   You should have received a copy of the LGPL along with this
   program. If not, go to http://www.gnu.org/licenses/lgpl.html
   or write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

On Debian systems, the complete text of the GNU Lesser General Public
License can be found in `/usr/share/common-licenses/LGPL-3'.")

execute_process(COMMAND gzip -9 -c ${CMAKE_SOURCE_DIR}/docs/changelog
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  OUTPUT_FILE "${CMAKE_BINARY_DIR}/changelog.Debian.gz")

install(FILES "${CMAKE_BINARY_DIR}/changelog.Debian.gz"
  "${CMAKE_BINARY_DIR}/copyright"
  "${CMAKE_SOURCE_DIR}/docs/documentation.html"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)

include(CPack)
