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
#include <optional>

#include <glibmm.h>
#include <gstreamermm.h>

class Thumbnailer;

struct VideoProcessorOptions
{
  std::optional<int> width = {};
  std::optional<int> height = {};
  bool keep_aspect_ratio = true;
};

class VideoProcessor final
{
public:
  VideoProcessor(Glib::RefPtr<Glib::MainLoop> mainloop,
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
  bool on_bus_message(Glib::RefPtr<Gst::Bus> const& bus,
                      Glib::RefPtr<Gst::Message> const& message);
  void on_preroll_handoff(Glib::RefPtr<Gst::Buffer> const& buffer,
                          Glib::RefPtr<Gst::Pad> const& pad);
  void shutdown();
  void queue_shutdown();

  bool on_timeout();

private:
  Glib::RefPtr<Glib::MainLoop> m_mainloop;
  Thumbnailer& m_thumbnailer;

  Glib::RefPtr<Gst::Pipeline> m_pipeline;
  Glib::RefPtr<Gst::FakeSink> m_fakesink;

  std::vector<gint64> m_thumbnailer_pos;

  bool m_done;
  bool m_running;
  guint m_timeout_id;
  int  m_timeout;
  bool m_accurate;
  guint64 m_last_screenshot;

  VideoProcessorOptions m_opts;

private:
  VideoProcessor(const VideoProcessor&) = delete;
  VideoProcessor& operator=(const VideoProcessor&) = delete;
};

#endif

/* EOF */
