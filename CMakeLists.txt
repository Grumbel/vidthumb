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

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GST REQUIRED gstreamer-1.0)
pkg_search_module(GST_PBUTILS REQUIRED gstreamer-pbutils-1.0)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(CAIROMM REQUIRED cairomm-1.0)

set(WARNINGS 0 CACHE BOOL "Switch on extra warnings")

if(WARNINGS)
  set(WARNING_CXX_FLAGS
    -pedantic
    -Wall
    -Wextra
    -Wno-c++0x-compat
    -Wnon-virtual-dtor
    -Weffc++
    -Wconversion
    -Werror
    -Wshadow
    -Wcast-qual
    -Winit-self # only works with >= -O1
    -Wno-unused-parameter
    -Wfloat-equal
    -Wunreachable-code)
endif()


add_compile_options(-std=c++1y)
include_directories(external/logmich/include/)

add_library(liblogmich external/logmich/src/log.cpp external/logmich/src/logger.cpp)
set_target_properties(liblogmich PROPERTIES OUTPUT_NAME logmich)
target_include_directories(liblogmich PUBLIC external/logmich/include/)
target_link_libraries(liblogmich PUBLIC boost_system boost_filesystem)

add_executable(mediainfo src/media_info.cpp)
target_compile_options(mediainfo PRIVATE ${WARNING_CXX_FLAGS})
target_include_directories(mediainfo SYSTEM PUBLIC
  liblogmich
  ${GST_INCLUDE_DIRS}
  ${GST_PBUTILS_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${CAIROMM_INCLUDE_DIRS})
target_link_LIBRARIES(mediainfo
  liblogmich
  fmt::fmt
  ${GST_LIBRARIES}
  ${GST_PBUTILS_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${CAIROMM_LIBRARIES})

add_executable(gstdiscover src/gstdiscover.c)
target_include_directories(gstdiscover SYSTEM PUBLIC
  ${GST_INCLUDE_DIRS}
  ${GST_PBUTILS_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${CAIROMM_INCLUDE_DIRS})
target_link_LIBRARIES(gstdiscover
  liblogmich
  ${GST_LIBRARIES}
  ${GST_PBUTILS_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${CAIROMM_LIBRARIES})

add_library(gtest STATIC ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/src/gtest-all.cc)
target_include_directories(gtest SYSTEM PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/)

# for filename in Glob("uitests/*_test.cpp", strings=True):
#     uitest_prog = env.Program(filename[:-4], filename)
#     Default(uitest_prog)

add_library(gtest_main STATIC ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/src/gtest_main.cc)
target_include_directories(gtest_main SYSTEM PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/)
target_link_libraries(gtest INTERFACE ${CMAKE_THREAD_LIBS_INIT})

file(GLOB TEST_VIDTHUMB_SOURCES_CXX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  tests/*_test.cpp)

add_executable(test_vidthumb ${TEST_VIDTHUMB_SOURCES_CXX})
target_compile_options(test_vidthumb PRIVATE ${WARNING_CXX_FLAGS})
target_link_libraries(test_vidthumb gtest gtest_main)

add_executable(vidthumb
  src/fourd_thumbnailer.cpp
  src/grid_thumbnailer.cpp
  src/directory_thumbnailer.cpp
  src/param_list.cpp
  src/video_processor.cpp
  src/vidthumb.cpp)
target_compile_options(vidthumb PRIVATE ${WARNING_CXX_FLAGS})
target_include_directories(vidthumb SYSTEM PUBLIC
  liblogmich
  ${GST_INCLUDE_DIRS}
  ${GST_PBUTILS_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${CAIROMM_INCLUDE_DIRS})
target_link_libraries(vidthumb
  liblogmich
  ${GST_LIBRARIES}
  ${GST_PBUTILS_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${CAIROMM_LIBRARIES})

# EOF #
