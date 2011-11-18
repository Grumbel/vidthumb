/*
**  VidThumb - Video Thumbnailer
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <glibmm/main.h>
#include <gstreamermm.h>
#include <cairomm/cairomm.h>

#include "grid_thumbnailer.hpp"
#include "thumbnailer.hpp"
#include "video_processor.hpp"

int main(int argc, char** argv)
{
  try
  {
    std::string input_filename;
    std::string output_filename;
    int rows = 4;
    int cols = 4;

#define NEXT_ARG                                                        \
    if (i >= argc-1)                                                    \
    {                                                                   \
      std::cout << argv[i] << " requires an argument" << std::endl;     \
      return EXIT_FAILURE;                                              \
    } else { i += 1; }

    for(int i = 1; i < argc; ++i)
    {
      if (strcmp(argv[i], "-o") == 0 ||
          strcmp(argv[i], "--output") == 0)
      {
        NEXT_ARG;
        output_filename = argv[i];
      }
      else if (strcmp(argv[i], "-c") == 0 ||
               strcmp(argv[i], "--cols") == 0)
      {
        NEXT_ARG;
        cols = atoi(argv[i]);
      }
      else if (strcmp(argv[i], "-r") == 0 ||
               strcmp(argv[i], "--rows") == 0)
      {
        NEXT_ARG;
        rows = atoi(argv[i]);
      }
      else
      {
        input_filename = argv[i];
      }
    }
#undef NEXT_ARG

    if (input_filename.empty())
    {
      std::cout << "Error: input filename required" << std::endl;
      return EXIT_FAILURE;
    }

    if (output_filename.empty())
    {
      std::cout << "Error: output filename required" << std::endl;
      return EXIT_FAILURE;
    }

    std::cout << "input:  " << input_filename << std::endl;
    std::cout << "output: " << output_filename << std::endl;

    Glib::RefPtr<Glib::MainLoop> mainloop = Glib::MainLoop::create();
    Gst::init(argc, argv);

    GridThumbnailer thumbnailer(cols, rows);
    VideoProcessor processor(mainloop, thumbnailer, input_filename);
    mainloop->run();

    thumbnailer.save(output_filename);
  }
  catch(const Gst::ParseError& err)
  {
    std::cout << "Error: " << err.what() << std::endl;
  }
  catch(const std::exception& err)
  {
    std::cout << "Error: " << err.what() << std::endl;
  }

  return 0;
}

/* EOF */
