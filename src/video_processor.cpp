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
                               Thumbnailer& thumbnailer,
                               const std::string& filename,
                               int timeout) :
  m_mainloop(mainloop),
  m_thumbnailer(thumbnailer),
  m_pipeline(),
  m_playbin(),
  m_fakesink(),
  m_thumbnailer_pos(),
  m_done(false),
  m_running(false),
  m_timeout(timeout),
  m_last_screenshot()
{
  // Setup a second pipeline to get the actual thumbnails
  GError* error = NULL;
  m_playbin = gst_parse_launch("filesrc name=mysource "
                               "  ! decodebin "
                               "  ! videoconvert "
                               //"  ! videoscale "
                               "  ! video/x-raw,format=RGB " //depth=24,bpp=32,pixel-aspect-ratio=1/1 " // ,width=180
                               "  ! fakesink name=mysink sync=False signal-handoffs=True", 
                               &error);
  if (error)
  {
    throw std::runtime_error(error->message);
  }

  m_pipeline = GST_PIPELINE(m_playbin);

  GstElement* source = gst_bin_get_by_name(GST_BIN(m_pipeline), "mysource");
  std::cout << "SOURC: " << source << std::endl;
  g_object_set(source, "location", filename.c_str(), NULL);

  m_fakesink = gst_bin_get_by_name(GST_BIN(m_pipeline), "mysink");


  GstBus* thumbnail_bus = gst_pipeline_get_bus(m_pipeline);

  gst_bus_add_signal_watch(thumbnail_bus);
  gst_bus_add_watch(thumbnail_bus,
                    [](GstBus* bus, GstMessage* message, gpointer user_data) -> gboolean
                    {
                      static_cast<VideoProcessor*>(user_data)->on_bus_message(message);
                      return true;
                    },
                    this);

  {
    GstState state;
    GstState pending;
    GstStateChangeReturn ret = gst_element_get_state(GST_ELEMENT(m_playbin), &state, &pending, 0);
    std::cout << "--STATE: " << ret << " " << to_string(state) << " " << to_string(pending) << std::endl;
  }

  gst_element_set_state(GST_ELEMENT(m_playbin), GST_STATE_PAUSED);
  {
    GstState state;
    GstState pending;
    GstStateChangeReturn ret = gst_element_get_state(GST_ELEMENT(m_playbin), &state, &pending, 0);
    std::cout << "--STATE: " << ret << " " << to_string(state) << " " << to_string(pending) << std::endl;
  }

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

VideoProcessor::~VideoProcessor()
{
  // FIXME: add some cleanup
}

void
VideoProcessor::send_buffer_probe()
{
  std::cout << "---------- send_buffer_probe()" << std::endl;
  GstPad* pad = gst_element_get_static_pad(m_fakesink, "sink");

  auto callback = [](GstPad* arg_pad, GstPadProbeInfo* arg_info, gpointer user_data) -> GstPadProbeReturn
    {
      static_cast<VideoProcessor*>(user_data)->on_buffer_probe(arg_pad, arg_info);
      return GST_PAD_PROBE_OK;
    };
  gst_pad_add_probe(pad, 
                    GST_PAD_PROBE_TYPE_BUFFER,
                    callback,
                    this,
                    NULL);
}

gint64
VideoProcessor::get_duration()
{
  // NOTE: See this an alternative way:
  // http://docs.gstreamer.com/display/GstSDK/Basic+tutorial+9%3A+Media+information+gathering

  GstQuery* query = gst_query_new_duration(GST_FORMAT_TIME);
  gboolean ret = gst_element_query(m_playbin, query);
  if (ret)
  {
    GstFormat format;
    gint64 duration;
    gst_query_parse_duration(query, &format, &duration);

    if (format == GST_FORMAT_TIME)
    {
      gst_object_unref(query);
      return duration;
    }
    else
    {
      gst_object_unref(query);
      throw std::runtime_error("error: could not get format");
    }
  }
  else
  {
    gst_object_unref(query);
    throw std::runtime_error("error: QUERY FAILURE");
  }
}

gint64
VideoProcessor::get_position()
{
  GstQuery* query = gst_query_new_position(GST_FORMAT_TIME);
  gboolean ret = gst_element_query(m_playbin, query);
  if (ret)
  {
    GstFormat format;
    gint64 position;
    gst_query_parse_position(query, &format, &position);

    if (format == GST_FORMAT_TIME)
    {
      gst_object_unref(query);
      return position;
    }
    else
    {
      gst_object_unref(query);
      throw std::runtime_error("error: could not get format");
    }
  }
  else
  {
    gst_object_unref(query);
    throw std::runtime_error("error: QUERY FAILURE");
  }
}

bool
VideoProcessor::seek_step()
{
  std::cout << "!!!!!!!!!!!!!!!! seek_step: " << m_thumbnailer_pos.size() << std::endl;

  if (!m_thumbnailer_pos.empty())
  {
    std::cout << "--> REQUEST SEEK: " << m_thumbnailer_pos.back() << std::endl;
    /*
      if (!m_pipeline->seek(1.0,
      GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
      //GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
      GST_SEEK_TYPE_SET, m_thumbnailer_pos.back(),
      GST_SEEK_TYPE_NONE, 0))
    */
    if (!gst_element_seek_simple(GST_ELEMENT(m_pipeline), 
                                 GST_FORMAT_TIME,
                                 static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                                 m_thumbnailer_pos.back()))
    {
      std::cout << ">>>>>>>>>>>>>>>>>>>> SEEK FAILURE <<<<<<<<<<<<<<<<<<" << std::endl;
    }

    m_thumbnailer_pos.pop_back();
  }
  else
  {
    std::cout << "---------- DONE ------------" << std::endl;

    auto callback = [](gpointer user_data) -> gboolean
      {
        return static_cast<VideoProcessor*>(user_data)->shutdown();
      };
    g_idle_add(callback, this);

    m_done = true;
  }

  return false;
}

bool
VideoProcessor::on_buffer_probe(GstPad* pad, GstPadProbeInfo* info)
{
  g_get_current_time(&m_last_screenshot);

  GstBuffer* buffer = GST_BUFFER(info->data);
  if (buffer)
  {
    GstCaps* caps = gst_pad_get_current_caps(pad);
    GstStructure* structure = gst_caps_get_structure(caps, 0);
    int width;
    int height;

    if (structure)
    {
      gst_structure_get_int(structure, "width",  &width);
      gst_structure_get_int(structure, "height", &height);
    }

    std::cout << "                  "
              << ">>>>>>>>>>>>>>>>>>>>>>>>> on_buffer_probe: " << GST_PAD_NAME(pad)
      //<< " " << buffer->get_size() << " " << get_position()
              << " " << width << "x" << height
              << std::endl;

    {
      Cairo::RefPtr<Cairo::ImageSurface> img = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, width, height);

      { // blit and flip color channels
#if 0
        unsigned char* op = img->get_data();
        unsigned char* ip  = buffer->get_data();
        int ostride = img->get_stride();
        int istride = buffer->get_size() / height;
        for(int y = 0; y < height; ++y)
        {
          for(int x = 0; x < width; ++x)
          {
            op[y*ostride + 4*x+0] = ip[y*istride + 4*x+2];
            op[y*ostride + 4*x+1] = ip[y*istride + 4*x+1];
            op[y*ostride + 4*x+2] = ip[y*istride + 4*x+0];
          }
        }
#endif
      }

      m_thumbnailer.receive_frame(img, get_position());
    }
    
    auto callback = [](gpointer user_data) -> gboolean
      {
        return static_cast<VideoProcessor*>(user_data)->seek_step();
      };
    g_idle_add(callback, this);
  }
  return true;
}

void
VideoProcessor::on_bus_message(GstMessage* msg)
{
  if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_ERROR)
  {
    std::cout << "Error: ";
    GError* gerror;
    gchar* debug;
    gst_message_parse_error(msg, &gerror, &debug);

    std::cout << GST_MESSAGE_SRC_NAME(msg) << ": "
              << gerror->message << std::endl;

    auto callback = [](gpointer user_data) -> gboolean
      {
    return static_cast<VideoProcessor*>(user_data)->shutdown();
  };
    g_idle_add(callback, this);
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STATE_CHANGED)
  {
    GstState oldstate;
    GstState newstate;
    GstState pending;
    gst_message_parse_state_changed(msg, &oldstate, &newstate, &pending);

    std::cout << "message: " << GST_MESSAGE_SRC_NAME(msg) << " old:" << to_string(oldstate) << " new:" << to_string(newstate) << std::endl;

    if (GST_ELEMENT(GST_MESSAGE_SRC(msg)) == m_fakesink)
    {
      if (newstate == GST_STATE_PAUSED)
      {
        std::cout << "                       --------->>>>>>> PAUSE" << std::endl;
      }

      if (newstate == GST_STATE_PAUSED)
      {
        if (!m_running)
        {
          std::cout << "##################################### ONLY ONCE: " << " ################" << std::endl;
          m_thumbnailer_pos = m_thumbnailer.get_thumbnail_pos(get_duration());
          std::reverse(m_thumbnailer_pos.begin(), m_thumbnailer_pos.end());
          m_running = true;
          seek_step();
          send_buffer_probe();
        }
      }
    }
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_EOS)
  {
    std::cout << "end of stream" << std::endl;
    g_idle_add([](gpointer user_data) -> gboolean
               {
                 return static_cast<VideoProcessor*>(user_data)->shutdown();
               }, 
               this);
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_TAG)
  {
    std::cout << "MESSAGE_TAG" << std::endl;
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
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_ASYNC_DONE)
  {
    std::cout << "MESSAGE_ASYNC_DONE" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STREAM_STATUS)
  {
    std::cout << "MESSAGE_STREAM_STATUS" << std::endl;

    GstStreamStatusType type;
    GstElement* owner;
    gst_message_parse_stream_status(msg, &type, &owner);

    std::cout << "StreamStatus:";
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
    std::cout << "MESSAGE_REQUEST_STATE" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_STEP_START)
  {
    std::cout << "MESSAGE_STEP_START" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_REQUEST_STATE)
  {
    std::cout << "MESSAGE_REQUEST_STATE" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_QOS)
  {
    std::cout << "MESSAGE_QOS" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_LATENCY)
  {
    std::cout << "MESSAGE_LATENCY" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_DURATION)
  {
    std::cout << "MESSAGE_DURATION" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_APPLICATION)
  {
    std::cout << "MESSAGE_APPLICATION" << std::endl;
  }
  else if (GST_MESSAGE_TYPE(msg) & GST_MESSAGE_ELEMENT)
  {
    std::cout << "MESSAGE_ELEMENT" << std::endl;
  }
  else
  {
    std::cout << "unknown message: " << GST_MESSAGE_TYPE(msg) << std::endl;
    g_idle_add([](gpointer user_data) -> gboolean
               {
                 return static_cast<VideoProcessor*>(user_data)->shutdown();
               }, 
               this);
  }
}

bool
VideoProcessor::shutdown()
{
  {
    GstState state;
    GstState pending;
    GstStateChangeReturn ret = gst_element_get_state(GST_ELEMENT(m_playbin), &state, &pending, 0);
    std::cout << "--STATE: " << ret << " " << to_string(state) << " " << to_string(pending) << std::endl;
  }

  std::cout << "Going to shutdown!!!!!!!!!!!" << std::endl;
  gst_element_set_state(GST_ELEMENT(m_playbin), GST_STATE_NULL);
  g_main_loop_quit(m_mainloop);
  return false;
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

    auto callback = [](gpointer user_data) -> gboolean
      {
        return static_cast<VideoProcessor*>(user_data)->shutdown();
      };
    g_idle_add(callback, this);
  }

  return true;
}

/* EOF */
