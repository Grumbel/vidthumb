##  VidThumb - Video Thumbnailer
##  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
##
##  This program is free software: you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation, either version 3 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program.  If not, see <http://www.gnu.org/licenses/>.

env = Environment(CXXFLAGS = [ "-O3", "-g3",
                               "-std=c++1y",
                               # "-ansi",
                               "-pedantic",
                               "-Wall",
                               "-Wextra",
                               "-Wno-c++0x-compat",
                               "-Wnon-virtual-dtor",
                               "-Weffc++",
                               "-Wconversion",
                               "-Werror",
                               "-Wshadow",
                               "-Wcast-qual",
                               "-Winit-self", # only works with >= -O1
                               "-Wno-unused-parameter"])

liblogmich = env.StaticLibrary("logmich", ["external/logmich/src/log.cpp",
                                           "external/logmich/src/logger.cpp"])
env.Append(LIBS = [liblogmich, "boost_system", "boost_filesystem"])
env.Append(CPPPATH = "external/logmich/include/")

env.ParseConfig("pkg-config --cflags --libs gstreamer-1.0 | sed 's/-I/-isystem/g'")
env.ParseConfig("pkg-config --cflags --libs gstreamer-1.0 gstreamer-pbutils-1.0 | sed 's/-I/-isystem/g'")
env.ParseConfig("pkg-config --cflags --libs glib-2.0 | sed 's/-I/-isystem/g'")
env.ParseConfig("pkg-config --cflags --libs cairomm-1.0 | sed 's/-I/-isystem/g'")

Default(env.Program("mediainfo", ["src/media_info.cpp"]))
Default(env.Program("gstdiscover", ["src/gstdiscover.c"]))

for filename in Glob("uitests/*_test.cpp", strings=True):
    uitest_prog = env.Program(filename[:-4], filename)
    Default(uitest_prog)

gtest_env = Environment()
gtest_env.Append(CXXFLAGS = ["-isystemexternal/gtest-1.7.0/include/",
                             "-isystemexternal/gtest-1.7.0/"])
gtest_env.Append(LIBS = ["pthread"])

libgtest = gtest_env.StaticLibrary("gtest", "external/gtest-1.7.0/src/gtest-all.cc")
libgtest_main = gtest_env.StaticLibrary("gtest_main", "external/gtest-1.7.0/src/gtest_main.cc")

test_prog = gtest_env.Program("test_vidthumb",
                              Glob("tests/*_test.cpp")
                              + libgtest_main
                              + libgtest)
Alias("test", test_prog)
Default("test")

Default(env.Program("vidthumb",
                    ["src/fourd_thumbnailer.cpp",
                     "src/grid_thumbnailer.cpp",
                     "src/directory_thumbnailer.cpp",
                     "src/param_list.cpp",
                     "src/video_processor.cpp",
                     "src/vidthumb.cpp"]))

# EOF #
