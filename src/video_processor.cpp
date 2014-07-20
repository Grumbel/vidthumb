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

#include "video_processor.hpp"

#include <algorithm>
#include <assert.h>
#include <cairomm/cairomm.h>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "thumbnailer.hpp"

std::string to_string(GstState state)
{
  switch(state)
  {
    case GST_STATE_VOID_PENDING:
      return "pending";
    case GST_STATE_NULL:
      return "null";
    case GST_STATE_READY:
      return "ready";
    case GST_STATE_PAUSED:
      return "paused";
    case GST_STATE_PLAYING:
      return "playing";
    default:
      return "unknown";
  }
}

VideoProcessor::VideoProcessor(GMainLoop* mainloop,
                               Thumbnailer& thumbnailer) :
  m_mainloop(mainloop),
  m_thumbnailer(thumbnailer),
  m_pipeline(),
  m_fakesink(),
  m_thumbnailer_pos(),
  m_done(false),
  m_running(false),
  m_timeout(-1),
  m_accurate(false),
  m_last_screenshot()
{
  GError* error = NULL;
  m_pipeline = GST_PIPELINE(gst_parse_launch("filesrc name=mysource "
                                             "  ! decodebin "
                                             "  ! videoconvert "
                                             //"  ! videoscale "
                                             "  ! video/x-raw,format=BGRx " //depth=24,bpp=32,pixel-aspect-ratio=1/1 " // ,width=180
                                             "  ! fakesink name=mysink signal-handoffs=True", //sync=False
                                             &error));
  if (error)
  {
    throw std::runtime_error(error->message);
  }

  m_fakesink = gst_bin_get_by_name(GST_BIN(m_pipeline), "mysink");

  GstBus* thumbnail_bus = gst_pipeline_get_bus(m_pipeline);

  // intercept the frame data and makes thumbnails
  void (*callback3)(GstElement *fakesink, GstBuffer *buffer, GstPad *pad, gpointer user_data)
    = [](GstElement *fakesink, GstBuffer *buffer, GstPad *pad, gpointer user_data)
    {
      static_cast<VideoProcessor*>(user_data)->on_preroll_handoff(fakesink, buffer, pad);
    };
  g_signal_connect(m_fakesink, "preroll-handoff",
                   G_CALLBACK(callback3), this);

  // listen to bus messages
  gst_bus_add_watch(thumbnail_bus,
                    [](GstBus* bus, GstMessage* message, gpointer user_data) -> gboolean
                    {
                      static_cast<VideoProcessor*>(user_data)->on_bus_message(message);
                      return true;
                    },
                    this);

  g_object_unref(thumbnail_bus);
}

VideoProcessor::~VideoProcessor()
{
  g_object_unref(m_fakesink);
  g_object_unref(m_pipeline);
}

void
VideoProcessor::set_accurate(bool accurate)
{
  m_accurate = accurate;
}

void
VideoProcessor::set_timeout(int timeout)
{
  m_timeout = timeout;

  if (m_timeout != -1)
  {
    g_get_current_time(&m_last_screenshot);
    std::cout << "------------------------------------ install time out " << std::endl;
    auto callback = [](gpointer user_data) -> gboolean
      {
        return static_cast<VideoProcessor*>(user_data)->on_timeout();
      };
    g_timeout_add(m_timeout, callback, this);
  }
}

void
VideoProcessor::open(const std::string& filename)
{
  GstElement* source = gst_bin_get_by_name(GST_BIN(m_pipeline), "mysource");
  g_object_set(source, "location", filename.c_str(), NULL);
  g_object_unref(source);

  // bring stream into pause state so that the thumbnailing can begin
  gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_PAUSED);  
}

gint64
VideoProcessor::get_duration()
{
  // NOTE: See this an alternative way:
  // http://docs.gstreamer.com/display/GstSDK/Basic+tutorial+9%3A+Media+information+gathering

  GstQuery* query = gst_query_new_duration(GST_FORMAT_TIME);
  gboolean ret = gst_element_query(GST_ELEMENT(m_pipeline), query);
  if (ret)
  {
    GstFormat format;
    gint64 duration;
    gst_query_parse_duration(query, &format, &duration);

    if (format == GST_FORMAT_TIME)
    {
      gst_query_unref(query);
      return duration;
    }
    else
    {
      gst_query_unref(query);
      throw std::runtime_error("error: could not get format");
    }
  }
  else
  {
    gst_query_unref(query);
    throw std::runtime_error("error: QUERY FAILURE");
  }
}

gint64
VideoProcessor::get_position()
{
  GstQuery* query = gst_query_new_position(GST_FORMAT_TIME);
  gboolean ret = gst_element_query(GST_ELEMENT(m_pipeline), query);
  if (ret)
  {
    GstFormat format;
    gint64 position;
    gst_query_parse_position(query, &format, &position);

    if (format == GST_FORMAT_TIME)
    {
      gst_query_unref(query);
      return position;
    }
    else
    {
      gst_query_unref(query);
      throw std::runtime_error("error: could not get format");
    }
  }
  else
  {
    gst_query_unref(query);
    throw std::runtime_error("error: QUERY FAILURE");
  }
}

void
VideoProcessor::seek_step()
{
  std::cout << "!!!!!!!!!!!!!!!! seek_step: " << m_thumbnailer_pos.size() << std::endl;

  if (!m_thumbnailer_pos.empty())
  {
    GstSeekFlags seek_flags;
    if (m_accurate)
    {
      seek_flags = static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);  // slow
    }
    else
    {
      seek_flags = static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_NEAREST);  // fast
    }

    std::cout << "--> REQUEST SEEK: " << m_thumbnailer_pos.back() << std::endl;
    if (!gst_element_seek_simple(GST_ELEMENT(m_pipeline),
                                 GST_FORMAT_TIME,
                                 seek_flags,
                                 m_thumbnailer_pos.back()))
    {
      std::cout << ">>>>>>>>>>>>>>>>>>>> SEEK FAILURE <<<<<<<<<<<<<<<<<<" << std::endl;
    }

    m_thumbnailer_pos.pop_back();
  }
  else
  {
    std::cout << "---------- DONE ------------" << std::endl;

    queue_shutdown();

    m_done = true;
  }
}

Cairo::RefPtr<Cairo::ImageSurface> buffer2cairo(GstBuffer* buffer, GstPad* pad)
{
  GstCaps* caps = gst_pad_get_current_caps(pad);
  GstStructure* structure = gst_caps_get_structure(caps, 0);

  int width, height;
  gst_structure_get_int(structure, "width",  &width);
  gst_structure_get_int(structure, "height", &height);

  Cairo::RefPtr<Cairo::ImageSurface> img = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, width, height);

  unsigned char* op = img->get_data();

  int ostride = img->get_stride();
  int istride = static_cast<int>(gst_buffer_get_size(buffer) / height);
  for(int y = 0; y < height; ++y)
  {
    gst_buffer_extract(buffer, y * istride,
                       op + y * ostride,
                       width * 4);
  }

  gst_caps_unref(caps);

  return img;
}

void
VideoProcessor::on_preroll_handoff(GstElement* fakesink, GstBuffer* buffer, GstPad* pad)
{
  if (m_running)
  {
    std::cout << "-------- preroll-handoff: " << buffer << " " << pad << std::endl;
    g_get_current_time(&m_last_screenshot);
    auto img = buffer2cairo(buffer, pad);
    m_thumbnailer.receive_frame(img, get_position());

    auto callback = [](gpointer user_data) -> gboolean
      {
        static_cast<VideoProcessor*>(user_data)->seek_step();
        return false;
      };
    g_idle_add(callback, this);
    //g_timeout_add(100, callback, this);
  }
}

void
VideoProcessor::on_bus_message(GstMessage* msg)
{
  //std::cout << "VideoProcessor::on_bus_message" << std::endl;
  if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_ERROR)
  {
    std::cout << "Error: ";
    GError* gerror;
    gchar* debug;
    gst_message_parse_error(msg, &gerror, &debug);

    std::cout << GST_MESSAGE_SRC_NAME(msg) << ": "
              << gerror->message << std::endl;

    queue_shutdown();
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STATE_CHANGED)
  {
    GstState oldstate, newstate, pending;
    gst_message_parse_state_changed(msg, &oldstate, &newstate, &pending);

    std::cout << "GST_MESSAGE_STATE_CHANGED: " << GST_MESSAGE_SRC_NAME(msg) << " old:" << to_string(oldstate) << " new:" << to_string(newstate) << std::endl;

    if (GST_ELEMENT(GST_MESSAGE_SRC(msg)) == m_fakesink &&
        newstate == GST_STATE_PAUSED &&
        !m_running)
    {
      std::cout << "##################################### ONLY ONCE: " << " ################" << std::endl;
      m_thumbnailer_pos = m_thumbnailer.get_thumbnail_pos(get_duration());
      std::reverse(m_thumbnailer_pos.begin(), m_thumbnailer_pos.end());
      m_running = true;
      seek_step();
    }
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_EOS)
  {
    std::cout << "GST_MESSAGE_EOS" << std::endl;
    queue_shutdown();
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_TAG)
  {
    if (false)
    {
      std::cout << "GST_MESSAGE_TAG" << std::endl;
      GstTagList* tag_list;
      gst_message_parse_tag(msg, &tag_list);

      gst_tag_list_foreach(tag_list,
                           [](const GstTagList* list,
                              const gchar* tag,
                              gpointer user_data)
                           {
                             std::cout << "  tag: " << tag << std::endl;
                           },
                           this);
    }
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_ASYNC_DONE)
  {
    std::cout << "GST_MESSAGE_ASYNC_DONE" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STREAM_STATUS)
  {
    std::cout << "GST_MESSAGE_STREAM_STATUS: ";

    GstStreamStatusType type;
    GstElement* owner;
    gst_message_parse_stream_status(msg, &type, &owner);

    switch(type)
    {
      case GST_STREAM_STATUS_TYPE_CREATE:
        std::cout << "CREATE";
        break;

      case GST_STREAM_STATUS_TYPE_ENTER:
        std::cout << "ENTER";
        break;

      case GST_STREAM_STATUS_TYPE_LEAVE:
        std::cout << "LEAVE";
        break;

      case GST_STREAM_STATUS_TYPE_DESTROY:
        std::cout << "DESTROY";
        break;

      case GST_STREAM_STATUS_TYPE_START:
        std::cout << "START";
        break;

      case GST_STREAM_STATUS_TYPE_PAUSE:
        std::cout << "PAUSE";
        break;

      case GST_STREAM_STATUS_TYPE_STOP:
        std::cout << "PAUSE";
        break;

      default:
        std::cout << "???";
        break;
    }
    std::cout << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_REQUEST_STATE)
  {
    std::cout << "GST_MESSAGE_REQUEST_STATE" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STEP_START)
  {
    std::cout << "GST_MESSAGE_STEP_START" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_REQUEST_STATE)
  {
    std::cout << "GST_MESSAGE_REQUEST_STATE" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_QOS)
  {
    std::cout << "GST_MESSAGE_QOS" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_LATENCY)
  {
    std::cout << "GST_MESSAGE_LATENCY" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_DURATION)
  {
    std::cout << "GST_MESSAGE_DURATION" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_APPLICATION)
  {
    std::cout << "GST_MESSAGE_APPLICATION" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_ELEMENT)
  {
    std::cout << "GST_MESSAGE_ELEMENT" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_RESET_TIME)
  {
    std::cout << "GST_MESSAGE_RESET_TIME" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STREAM_START)
  {
    std::cout << "GST_MESSAGE_STREAM_START" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_NEED_CONTEXT)
  {
    std::cout << "GST_MESSAGE_NEED_CONTEXT" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_HAVE_CONTEXT)
  {
    std::cout << "GST_MESSAGE_HAVE_CONTEXT" << std::endl;
  }
  else
  {
    std::cout << "unknown message: " << GST_MESSAGE_TYPE(msg) << std::endl;
    queue_shutdown();
  }
}

void
VideoProcessor::queue_shutdown()
{
  auto callback = [](gpointer user_data) -> gboolean
    {
      static_cast<VideoProcessor*>(user_data)->shutdown();
      return false;
    };
  g_idle_add(callback, this);
}

void
VideoProcessor::shutdown()
{
  std::cout << "Going to shutdown!!!!!!!!!!!" << std::endl;
  gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_NULL);
  g_main_loop_quit(m_mainloop);
}

static double as_double(const GTimeVal& t)
{
  return double(t.tv_sec) + double(t.tv_usec) / double(G_USEC_PER_SEC);
}

static void subtract(GTimeVal& lhs, const GTimeVal& rhs)
{
  g_return_if_fail(lhs.tv_usec >= 0 && lhs.tv_usec < G_USEC_PER_SEC);
  g_return_if_fail(rhs.tv_usec >= 0 && rhs.tv_usec < G_USEC_PER_SEC);

  lhs.tv_usec -= rhs.tv_usec;

  if(lhs.tv_usec < 0)
  {
    lhs.tv_usec += G_USEC_PER_SEC;
    --lhs.tv_sec;
  }

  lhs.tv_sec -= rhs.tv_sec;
}

bool
VideoProcessor::on_timeout()
{
  GTimeVal t;

  g_get_current_time(&t);

  subtract(t, m_last_screenshot);

  std::cout << "TIMEOUT: " << as_double(t) << " " << m_timeout << std::endl;
  if (as_double(t) > m_timeout/1000.0)
  {
    std::cout << "--------- timeout ----------------: "  << as_double(t) << std::endl;
    queue_shutdown();
  }

  return true;
}

/* EOF */
