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
#include <sstream>
#include <stdexcept>
#include <vector>
#include <logmich.hpp>

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
  m_timeout_id(0),
  m_timeout(-1),
  m_accurate(false),
  m_last_screenshot(),
  m_opts()
{
}

VideoProcessor::~VideoProcessor()
{
  if (m_timeout_id)
  {
    g_source_remove(m_timeout_id);
    m_timeout_id = 0;
  }

  g_object_unref(m_fakesink);
  g_object_unref(m_pipeline);
}

std::string
VideoProcessor::get_pipeline_desc() const
{
  std::ostringstream pipeline_desc;

  pipeline_desc <<
    "filesrc name=mysource "
    "  ! decodebin "
    "  ! videoscale "
    "  ! videoconvert ";

  // force output format
  pipeline_desc << "  ! video/x-raw,format=BGRx";
  if (m_opts.width)
  {
    pipeline_desc << ",width=" << *m_opts.width;
  }

  if (m_opts.height)
  {
    pipeline_desc << ",height=" << *m_opts.height;
  }

  if (m_opts.keep_aspect_ratio)
  {
    pipeline_desc << ",pixel-aspect-ratio=1/1";
  }

  pipeline_desc << "  ! fakesink name=mysink signal-handoffs=True sync=False ";

  return pipeline_desc.str();
}

void
VideoProcessor::setup_pipeline()
{
  auto pipeline_desc = get_pipeline_desc();
  log_info("Using pipeline: %s", pipeline_desc);
  GError* error = nullptr;
  m_pipeline = GST_PIPELINE(gst_parse_launch(pipeline_desc.c_str(), &error));
  if (error)
  {
    std::runtime_error exception(error->message);
    g_error_free(error);
    throw exception;
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

void
VideoProcessor::set_accurate(bool accurate)
{
  m_accurate = accurate;
}

void
VideoProcessor::set_options(const VideoProcessorOptions opts)
{
  m_opts = opts;
}

void
VideoProcessor::set_timeout(int timeout)
{
  if (m_timeout_id)
  {
    g_source_remove(m_timeout_id);
    m_timeout_id = 0;
  }

  m_timeout = timeout;

  if (m_timeout != -1)
  {
    g_get_current_time(&m_last_screenshot);
    log_info("------------------------------------ install time out " );
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
  setup_pipeline();

  GstElement* source = gst_bin_get_by_name(GST_BIN(m_pipeline), "mysource");
  g_object_set(source, "location", filename.c_str(), nullptr);
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
  log_info("!!!!!!!!!!!!!!!! seek_step: %s", m_thumbnailer_pos.size());

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

    log_info("--> REQUEST SEEK: %s", m_thumbnailer_pos.back());
    if (!gst_element_seek_simple(GST_ELEMENT(m_pipeline),
                                 GST_FORMAT_TIME,
                                 seek_flags,
                                 m_thumbnailer_pos.back()))
    {
      log_info(">>>>>>>>>>>>>>>>>>>> SEEK FAILURE <<<<<<<<<<<<<<<<<<");
    }

    m_thumbnailer_pos.pop_back();
  }
  else
  {
    log_info("---------- DONE ------------");

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
    g_get_current_time(&m_last_screenshot);
    auto img = buffer2cairo(buffer, pad);
    m_thumbnailer.receive_frame(img, get_position());

    auto callback = [](gpointer user_data) -> gboolean
      {
        static_cast<VideoProcessor*>(user_data)->seek_step();
        return false;
      };
    g_idle_add(callback, this);
  }
}

void
VideoProcessor::on_bus_message(GstMessage* msg)
{
  //log_info("VideoProcessor::on_bus_message");
  if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_ERROR)
  {
    GError* gerror = nullptr;
    gchar* debug = nullptr;
    gst_message_parse_error(msg, &gerror, &debug);

    log_info("Error: %s: %s", GST_MESSAGE_SRC_NAME(msg), gerror->message);

    queue_shutdown();

    g_error_free(gerror);
    g_free(debug);
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STATE_CHANGED)
  {
    GstState oldstate, newstate, pending;
    gst_message_parse_state_changed(msg, &oldstate, &newstate, &pending);

    log_debug("GST_MESSAGE_STATE_CHANGED: %s old: %s new: %s",
              GST_MESSAGE_SRC_NAME(msg),
              to_string(oldstate),
              to_string(newstate));

    if (GST_ELEMENT(GST_MESSAGE_SRC(msg)) == m_fakesink &&
        newstate == GST_STATE_PAUSED &&
        !m_running)
    {
      log_info("##################################### ONLY ONCE: ################");
      m_thumbnailer_pos = m_thumbnailer.get_thumbnail_pos(get_duration());
      std::reverse(m_thumbnailer_pos.begin(), m_thumbnailer_pos.end());
      m_running = true;
      seek_step();
    }
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_EOS)
  {
    log_debug("GST_MESSAGE_EOS");
    queue_shutdown();
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_TAG)
  {
    if (false)
    {
      log_debug("GST_MESSAGE_TAG");
      GstTagList* tag_list = nullptr;
      gst_message_parse_tag(msg, &tag_list);

      gst_tag_list_foreach(tag_list,
                           [](const GstTagList* list,
                              const gchar* tag,
                              gpointer user_data)
                           {
                             log_info("  tag: %s", tag);
                           },
                           this);

      gst_tag_list_unref(tag_list);
    }
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_ASYNC_DONE)
  {
    log_debug("GST_MESSAGE_ASYNC_DONE");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STREAM_STATUS)
  {
    log_debug("GST_MESSAGE_STREAM_STATUS: ");

    GstStreamStatusType type;
    GstElement* owner = nullptr;
    gst_message_parse_stream_status(msg, &type, &owner);

    switch(type)
    {
      case GST_STREAM_STATUS_TYPE_CREATE:
        log_info("CREATE");
        break;

      case GST_STREAM_STATUS_TYPE_ENTER:
        log_info("ENTER");
        break;

      case GST_STREAM_STATUS_TYPE_LEAVE:
        log_info("LEAVE");
        break;

      case GST_STREAM_STATUS_TYPE_DESTROY:
        log_info("DESTROY");
        break;

      case GST_STREAM_STATUS_TYPE_START:
        log_info("START");
        break;

      case GST_STREAM_STATUS_TYPE_PAUSE:
        log_info("PAUSE");
        break;

      case GST_STREAM_STATUS_TYPE_STOP:
        log_info("PAUSE");
        break;

      default:
        log_info("???");
        break;
    }
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_REQUEST_STATE)
  {
    log_debug("GST_MESSAGE_REQUEST_STATE");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STEP_START)
  {
    log_debug("GST_MESSAGE_STEP_START");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_REQUEST_STATE)
  {
    log_debug("GST_MESSAGE_REQUEST_STATE");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_QOS)
  {
    log_debug("GST_MESSAGE_QOS");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_LATENCY)
  {
    log_debug("GST_MESSAGE_LATENCY");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_DURATION)
  {
    log_debug("GST_MESSAGE_DURATION");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_APPLICATION)
  {
    log_debug("GST_MESSAGE_APPLICATION");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_ELEMENT)
  {
    log_debug("GST_MESSAGE_ELEMENT");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_RESET_TIME)
  {
    log_debug("GST_MESSAGE_RESET_TIME");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STREAM_START)
  {
    log_debug("GST_MESSAGE_STREAM_START");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_NEED_CONTEXT)
  {
    log_debug("GST_MESSAGE_NEED_CONTEXT");
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_HAVE_CONTEXT)
  {
    log_debug("GST_MESSAGE_HAVE_CONTEXT");
  }
  else
  {
    log_info("unknown message: %s", GST_MESSAGE_TYPE(msg));
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
  log_info("Going to shutdown!!!!!!!!!!!");
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

  log_info("TIMEOUT: %s %s", as_double(t), m_timeout);
  if (as_double(t) > m_timeout/1000.0)
  {
    log_info("--------- timeout ----------------: %s", as_double(t));
    queue_shutdown();
  }

  return true;
}

/* EOF */
