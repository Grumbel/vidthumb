# VidThumb - Video Thumbnailer
# Copyright (C) 2015 Ingo Ruhnke <grumbel@gmx.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.0)
project(vidthumb)

include(mk/cmake/TinyCMMC.cmake)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(fmt REQUIRED)
pkg_search_module(GLIBMM REQUIRED glibmm-2.4 IMPORTED_TARGET)
pkg_search_module(CAIROMM REQUIRED cairomm-1.0 IMPORTED_TARGET)
pkg_search_module(GSTREAMERMM REQUIRED gstreamermm-1.0 IMPORTED_TARGET)

function(build_dependencies)
  set(BUILD_TESTS OFF)
  tinycmmc_find_dependency(logmich)
endfunction()
build_dependencies()

add_executable(mediainfo src/media_info.cpp)
target_compile_options(mediainfo PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
target_link_libraries(mediainfo PRIVATE
  logmich::logmich
  fmt::fmt
  PkgConfig::GSTREAMERMM
  PkgConfig::GLIBMM
  PkgConfig::CAIROMM)

if(BUILD_TESTS)
  enable_testing()

  find_package(GTest REQUIRED)

  file(GLOB TEST_VIDTHUMB_SOURCES_CXX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    tests/*_test.cpp)

  add_executable(test_vidthumb ${TEST_VIDTHUMB_SOURCES_CXX})
  target_compile_options(test_vidthumb PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
  target_link_libraries(test_vidthumb GTest::GTest GTest::Main)

  add_test(NAME test_vidthumb
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND test_vidthumb)
endif()

add_executable(vidthumb
  src/fourd_thumbnailer.cpp
  src/grid_thumbnailer.cpp
  src/directory_thumbnailer.cpp
  src/param_list.cpp
  src/video_processor.cpp
  src/vidthumb.cpp)
target_compile_options(vidthumb PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
target_link_libraries(vidthumb PRIVATE
  logmich::logmich
  fmt::fmt
  PkgConfig::GSTREAMERMM
  PkgConfig::GLIBMM
  PkgConfig::CAIROMM)

install(TARGETS vidthumb mediainfo
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/vidthumbzip.py
  RENAME vidthumbzip
  PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  DESTINATION "bin/")

# EOF #
