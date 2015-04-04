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

#ifndef HEADER_VIDEO_PROCESSOR_HPP
#define HEADER_VIDEO_PROCESSOR_HPP

#include <assert.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <boost/optional.hpp>

#include <glib.h>
#include <gst/gst.h>

class Thumbnailer;

struct VideoProcessorOptions
{
  boost::optional<int> width = {};
  boost::optional<int> height = {};
  bool keep_aspect_ratio = true;
};

class VideoProcessor final
{
private:
  GMainLoop* m_mainloop;
  Thumbnailer& m_thumbnailer;

  GstPipeline* m_pipeline;
  GstElement* m_fakesink;

  std::vector<gint64> m_thumbnailer_pos;

  bool m_done;
  bool m_running;
  guint m_timeout_id;
  int  m_timeout;
  bool m_accurate;
  GTimeVal m_last_screenshot;

  VideoProcessorOptions m_opts;

public:
  VideoProcessor(GMainLoop* mainloop,
                 Thumbnailer& thumbnailer);
  ~VideoProcessor();

  void set_accurate(bool accurate);
  void set_timeout(int timeout);
  void set_options(const VideoProcessorOptions opts);
  void open(const std::string& filename);
  void setup_pipeline();
  std::string get_pipeline_desc() const;

  gint64 get_duration();
  gint64 get_position();

  void seek_step();
  void on_bus_message(GstMessage* msg);
  void on_preroll_handoff(GstElement *fakesink, GstBuffer *buffer, GstPad *pad);
  void shutdown();
  void queue_shutdown();

  bool on_timeout();

private:


private:
  VideoProcessor(const VideoProcessor&) = delete;
  VideoProcessor& operator=(const VideoProcessor&) = delete;
};

#endif

/* EOF */
