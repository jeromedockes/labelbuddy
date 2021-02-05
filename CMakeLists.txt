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

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME labelbuddy)
set(CPACK_PACKAGE_VERSION 0.0.1-1)
set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jerome Dockes <jerome@dockes.org>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "gui tool for annotating documents")

install(TARGETS labelbuddy
  RUNTIME DESTINATION "/usr/bin/")

include(CPack)
