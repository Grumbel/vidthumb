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

env = Environment(CXXFLAGS = [ "-O0", "-g3",
                               "-std=c++0x",
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

env.ParseConfig("pkg-config --cflags --libs gstreamermm-0.10 | sed 's/-I/-isystem/g'")
env.ParseConfig("pkg-config --cflags --libs cairomm-1.0 | sed 's/-I/-isystem/g'")
env.ParseConfig("pkg-config --cflags --libs glibmm-2.4 | sed 's/-I/-isystem/g'")
env.ParseConfig("pkg-config --cflags --libs gstreamer-0.10 | sed 's/-I/-isystem/g'")

# Glob('src/*.cpp'))
env.Program("vidthumb", ["src/fourd_thumbnailer.cpp",
                         "src/grid_thumbnailer.cpp",
                         "src/param_list.cpp",
                         "src/video_processor.cpp",
                         "src/vidthumb.cpp"])

env.Program("mediainfo", ["src/media_info.cpp"])

# EOF #
