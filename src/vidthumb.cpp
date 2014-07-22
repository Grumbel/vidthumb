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
#include <sstream>
#include <stdexcept>
#include <vector>

#include "fourd_thumbnailer.hpp"
#include "grid_thumbnailer.hpp"
#include "param_list.hpp"
#include "thumbnailer.hpp"
#include "video_processor.hpp"

class Options
{
public:
  std::string input_filename;
  std::string output_filename;
  VideoProcessorOptions vp_opts;
  int timeout;
  bool accurate;
  enum { kGridThumbnailer, kFourdThumbnailer } mode;
  ParamList params;

public:
  Options() :
    input_filename(),
    output_filename(),
    vp_opts(),
    timeout(5000),
    accurate(false),
    mode(kGridThumbnailer),
    params()
  {}

  void parse_args(int argc, char** argv);
};

void
Options::parse_args(int argc, char** argv)
{
#define NEXT_ARG                                                        \
    if (i >= argc-1)                                                    \
    {                                                                   \
      std::ostringstream out;                                           \
      out << argv[i] << " requires an argument";                        \
      throw std::runtime_error(out.str());                              \
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
          "  -W, --width INT        Rescale the video to width\n"
          "  -H, --height INT       Rescale the video to height\n"
          "  -A, --ignore-aspect-ratio\n"
          "                         Ignore aspect-ratio\n"
          "  -p, --params PARAMS    Pass additional parameter to the thumbnailer (e.g. cols=5,rows=3)\n"
          "  --fourd                Use fourd thumbnailer\n"
          "                           parameter: slices=INT\n"
          "  --grid                 Use grid thumbnailer (default)\n"
          "                           parameter: cols=INT,rows=INT\n"
          "  -t, --timeout SECONDS  Wait for SECONDS before giving up, -1 for infinity\n"
          "  -T, --timestamp        Timestamp the frames\n"
          "  -a, --accurate         Use accurate, but slow seeking\n";
        exit(0);
      }
      else if (strcmp(argv[i], "-o") == 0 ||
               strcmp(argv[i], "--output") == 0)
      {
        NEXT_ARG;
        output_filename = argv[i];
      }
      else if (strcmp(argv[i], "-W") == 0 ||
               strcmp(argv[i], "--width") == 0)
      {
        NEXT_ARG;
        vp_opts.width = atoi(argv[i]);
      }
      else if (strcmp(argv[i], "-H") == 0 ||
               strcmp(argv[i], "--height") == 0)
      {
        NEXT_ARG;
        vp_opts.height = atoi(argv[i]);
      }
      else if (strcmp(argv[i], "-A") == 0 ||
               strcmp(argv[i], "--ignore-aspect-ratio") == 0)
      {
        vp_opts.keep_aspect_ratio = false;
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
      else if (strcmp(argv[i], "--timestamp") == 0 ||
               strcmp(argv[i], "-T") == 0)
      {
        // FIXME: implement me
      }
      else
      {
        input_filename = argv[i];
      }
    }
#undef NEXT_ARG

    if (input_filename.empty())
    {
      throw std::runtime_error("input filename required");
    }

    if (output_filename.empty())
    {
      throw std::runtime_error("output filename required");
    } 
}

int main(int argc, char** argv)
{
  try
  {
    Options opts;
    opts.parse_args(argc, argv);

    std::cout << "input:  " << opts.input_filename << std::endl;
    std::cout << "output: " << opts.output_filename << std::endl;

    gst_init(&argc, &argv);

    std::unique_ptr<Thumbnailer> thumbnailer;
    switch(opts.mode)
    {
      case Options::kGridThumbnailer: {
        int cols = 4;
        int rows = 4;
        opts.params.get("cols", &cols);
        opts.params.get("rows", &rows);
        thumbnailer.reset(new GridThumbnailer(cols, rows));
        break;
      }

      case Options::kFourdThumbnailer: {
        int slices = 100;
        opts.params.get("slices", &slices);
        thumbnailer.reset(new FourdThumbnailer(slices));
        break;
      }

      default:
        assert(!"never reached");
        break;
    }

    gst_init(&argc, &argv);
    GMainLoop* mainloop = g_main_loop_new(NULL, false);
    {
      VideoProcessor processor(mainloop, *thumbnailer);
      processor.set_options(opts.vp_opts);
      processor.set_timeout(opts.timeout);
      processor.set_accurate(opts.accurate);
      processor.open(opts.input_filename);
      g_main_loop_run(mainloop);
      thumbnailer->save(opts.output_filename);
    }
    g_main_loop_unref(mainloop);
    gst_deinit();
  }
  catch(const std::exception& err)
  {
    std::cerr << "Exception: " << err.what() << std::endl;
  }

  return 0;
}

/* EOF */
