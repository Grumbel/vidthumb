/*
**  VidThumb - Video Thumbnailer
**  Copyright (C) 2011,2014 Ingo Ruhnke <grumbel@gmx.de>
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

#include <algorithm>
#include <assert.h>
#include <cairomm/cairomm.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "fourd_thumbnailer.hpp"
#include "grid_thumbnailer.hpp"
#include "param_list.hpp"
#include "thumbnailer.hpp"
#include "video_processor.hpp"

int main(int argc, char** argv)
{
  try
  {
    std::string input_filename;
    std::string output_filename;
    int timeout = 5000;
    bool accurate = false;
    enum { kGridThumbnailer, kFourdThumbnailer } mode = kGridThumbnailer;
    ParamList params;

#define NEXT_ARG                                                        \
    if (i >= argc-1)                                                    \
    {                                                                   \
      std::cout << argv[i] << " requires an argument" << std::endl;     \
      return EXIT_FAILURE;                                              \
    } else { i += 1; }

    for(int i = 1; i < argc; ++i)
    {
      if (strcmp(argv[i], "-h") == 0 ||
          strcmp(argv[i], "--help") == 0)
      {
        std::cout << "Usage: " << argv[0] << " [OPTIONS] FILENAME" << std::endl;
        std::cout << std::endl;
        std::cout <<
          "  -o, --output FILE      Write thumbnail to FILE\n"
          "  -p, --params PARAMS    Pass additional parameter to the thumbnailer (e.g. cols=5,rows=3)\n"
          "  --fourd                Use fourd thumbnailer\n"
          "                           parameter: slices=INT\n"
          "  --grid                 Use grid thumbnailer (default)\n"
          "                           parameter: cols=INT,rows=INT\n"
          "  -t, --timeout SECONDS  Wait for SECONDS before giving up, -1 for infinity\n"
          "  -a, --accurate         Use accurate, but slow seeking\n";
        exit(0);
      }
      else if (strcmp(argv[i], "-o") == 0 ||
               strcmp(argv[i], "--output") == 0)
      {
        NEXT_ARG;
        output_filename = argv[i];
      }
      else if (strcmp(argv[i], "-p") == 0 ||
               strcmp(argv[i], "--params") == 0)
      {
        NEXT_ARG;
        params.parse_string(argv[i]);
      }
      else if (strcmp(argv[i], "--fourd") == 0)
      {
        mode = kFourdThumbnailer;
      }
      else if (strcmp(argv[i], "--grid") == 0)
      {
        mode = kGridThumbnailer;
      }
      else if (strcmp(argv[i], "--timeout") == 0 ||
               strcmp(argv[i], "-t") == 0)
      {
        NEXT_ARG;
        timeout = static_cast<int>(atof(argv[i]) * 1000.0);
      }
      else if (strcmp(argv[i], "--accurate") == 0 ||
               strcmp(argv[i], "-a") == 0)
      {
        accurate = true;
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

    GMainLoop* mainloop = g_main_loop_new(NULL, false);
    gst_init(&argc, &argv);

    std::unique_ptr<Thumbnailer> thumbnailer;
    switch(mode)
    {
      case kGridThumbnailer: {
        int cols = 4;
        int rows = 4;
        params.get("cols", &cols);
        params.get("rows", &rows);
        thumbnailer.reset(new GridThumbnailer(cols, rows));
        break;
      }

      case kFourdThumbnailer: {
        int slices = 100;
        params.get("slices", &slices);
        thumbnailer.reset(new FourdThumbnailer(slices));
        break;
      }

      default:
        assert(!"never reached");
        break;
    }

    {
      VideoProcessor processor(mainloop, *thumbnailer);
      processor.set_timeout(timeout);
      processor.set_accurate(accurate);
      processor.open(input_filename);
      g_main_loop_run(mainloop);
    }

    g_main_loop_unref(mainloop);
    gst_deinit();

    thumbnailer->save(output_filename);
  }
  catch(const std::exception& err)
  {
    std::cout << "Error: " << err.what() << std::endl;
  }

  return 0;
}

/* EOF */
