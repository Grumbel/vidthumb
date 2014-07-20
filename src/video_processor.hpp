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

#include <glib.h>
#include <gst/gst.h>

class Thumbnailer;

class VideoProcessor final
{
private:
  GMainLoop* m_mainloop;
  Thumbnailer& m_thumbnailer;

  GstPipeline* m_pipeline;
  GstElement* m_playbin;
  GstElement* m_fakesink;

  std::vector<gint64> m_thumbnailer_pos;

  bool m_done;
  bool m_running;
  int  m_timeout;
  GTimeVal m_last_screenshot;

public:
  VideoProcessor(GMainLoop* mainloop,
                 Thumbnailer& thumbnailer,
                 const std::string& filename,
                 int timeout = -1);
  ~VideoProcessor();

  void send_buffer_probe();
  gint64 get_duration();
  gint64 get_position();
  bool seek_step();
  bool on_buffer_probe(GstPad* pad, GstPadProbeInfo *info);
  void on_bus_message(GstMessage* msg);
  bool shutdown();
  bool on_timeout();

private:
  VideoProcessor(const VideoProcessor&) = delete;
  VideoProcessor& operator=(const VideoProcessor&) = delete;
};

#endif

/* EOF */
