env = Environment(CXXFLAGS = [ "-O0", "-g3",
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

env.Program("main", Glob("src/*.cpp"))

# EOF #
