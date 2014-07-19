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
#include <glibmm/main.h>
#include <gstreamermm.h>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "thumbnailer.hpp"

std::string to_string(Gst::State state)
{
  switch(state)
  {
    case Gst::STATE_VOID_PENDING:
      return "pending";
    case Gst::STATE_NULL:
      return "null";
    case Gst::STATE_READY:
      return "ready";
    case Gst::STATE_PAUSED:
      return "paused";
    case Gst::STATE_PLAYING:
      return "playing";
    default:
      return "unknown";
  }
}

VideoProcessor::VideoProcessor(Glib::RefPtr<Glib::MainLoop> mainloop,
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
  m_playbin = Gst::Parse::launch("filesrc name=mysource "
                                 "  ! decodebin2 name=src "
                                 "src. "
                                 "  ! queue "
                                 "  ! ffmpegcolorspace "
                                 "  ! videoscale "
                                 "  ! video/x-raw-rgb,depth=24,bpp=32,pixel-aspect-ratio=1/1 " // ,width=180
                                 "  ! fakesink name=mysink sync=False signal-handoffs=True");

  m_pipeline = Glib::RefPtr<Gst::Pipeline>::cast_dynamic(m_playbin);

  Glib::RefPtr<Gst::Element> source = m_pipeline->get_element("mysource");
  std::cout << "SOURC: " << source << std::endl;
  source->set_property("location", filename);

  m_fakesink = m_pipeline->get_element("mysink");

  Glib::RefPtr<Gst::Bus> thumbnail_bus = m_playbin->get_bus();
  thumbnail_bus->add_signal_watch();
  thumbnail_bus->signal_message().connect(sigc::mem_fun(this, &VideoProcessor::on_bus_message));

  {
    Gst::State state;
    Gst::State pending;
    Gst::StateChangeReturn ret = m_playbin->get_state(state, pending, 0);
    std::cout << "--STATE: " << ret << " " << to_string(state) << " " << to_string(pending) << std::endl;
  }
  m_playbin->set_state(Gst::STATE_PAUSED);
  {
    Gst::State state;
    Gst::State pending;
    Gst::StateChangeReturn ret = m_playbin->get_state(state, pending, 0);
    std::cout << "--STATE: " << ret << " " << to_string(state) << " " << to_string(pending) << std::endl;
  }

  if (m_timeout != -1)
  {
    m_last_screenshot.assign_current_time();
    std::cout << "------------------------------------ install time out " << std::endl;
    Glib::signal_timeout().connect(sigc::mem_fun(this, &VideoProcessor::on_timeout), m_timeout);
  }
}

void
VideoProcessor::send_buffer_probe()
{
  std::cout << "---------- send_buffer_probe()" << std::endl;
  Glib::RefPtr<Gst::Pad> pad = m_fakesink->get_static_pad("sink");
  pad->add_buffer_probe(sigc::mem_fun(this, &VideoProcessor::on_buffer_probe));
}

gint64
VideoProcessor::get_duration()
{
  Gst::Format format = Gst::FORMAT_TIME;
  gint64 duration;
  if (m_playbin->query_duration(format, duration))
  {
    if (format == Gst::FORMAT_TIME)
    {
      return duration;
    }
    else
    {
      throw std::runtime_error("error: could not get format");
    }
  }
  else
  {
    throw std::runtime_error("error: QUERY FAILURE");
  }
}

gint64
VideoProcessor::get_position()
{
  Gst::Format format = Gst::FORMAT_TIME;
  gint64 position;
  if (m_playbin->query_position(format, position))
  {
    if (format == Gst::FORMAT_TIME)
    {
      return position;
    }
    else
    {
      std::cout << "error: could not get format" << std::endl;
      return 0;
    }
  }
  else
  {
    std::cout << "QUERY FAILURE" << std::endl;
    return 0;
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
                          Gst::FORMAT_TIME,
                          Gst::SEEK_FLAG_FLUSH | Gst::SEEK_FLAG_ACCURATE,
                          //Gst::SEEK_FLAG_FLUSH | Gst::SEEK_FLAG_KEY_UNIT,
                          Gst::SEEK_TYPE_SET, m_thumbnailer_pos.back(),
                          Gst::SEEK_TYPE_NONE, 0))
    */
    if (!m_pipeline->seek(Gst::FORMAT_TIME,
                          Gst::SEEK_FLAG_FLUSH | Gst::SEEK_FLAG_ACCURATE,
                          m_thumbnailer_pos.back()))
    {
      std::cout << ">>>>>>>>>>>>>>>>>>>> SEEK FAILURE <<<<<<<<<<<<<<<<<<" << std::endl;
    }

    m_thumbnailer_pos.pop_back();
  }
  else
  {
    std::cout << "---------- DONE ------------" << std::endl;
    Glib::signal_idle().connect(sigc::mem_fun(this, &VideoProcessor::shutdown));
    m_done = true;
  }

  return false;
}

bool
VideoProcessor::on_buffer_probe(const Glib::RefPtr<Gst::Pad>& pad, const Glib::RefPtr<Gst::MiniObject>& miniobj)
{
  m_last_screenshot.assign_current_time();

  Glib::RefPtr<Gst::Buffer> buffer = Glib::RefPtr<Gst::Buffer>::cast_dynamic(miniobj);
  if (buffer)
  {
    Glib::RefPtr<Gst::Caps> caps = buffer->get_caps();
    const Gst::Structure structure = caps->get_structure(0);
    int width;
    int height;

    if (structure)
    {
      structure.get_field("width",  width);
      structure.get_field("height", height);
    }

    std::cout << "                  "
              << ">>>>>>>>>>>>>>>>>>>>>>>>> on_buffer_probe: " << pad->get_name()
              << " " << buffer->get_size() << " " << get_position()
              << " " << width << "x" << height
              << std::endl;

    {
      Cairo::RefPtr<Cairo::ImageSurface> img = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, width, height);

      { // blit and flip color channels
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
      }

      m_thumbnailer.receive_frame(img, get_position());
    }

    Glib::signal_idle().connect(sigc::mem_fun(this, &VideoProcessor::seek_step));
  }
  return true;
}

void
VideoProcessor::on_bus_message(const Glib::RefPtr<Gst::Message>& msg)
{
  if (msg->get_message_type() & Gst::MESSAGE_ERROR)
  {
    Glib::RefPtr<Gst::MessageError> error_msg = Glib::RefPtr<Gst::MessageError>::cast_dynamic(msg);
    std::cout << "Error: "
              << msg->get_source()->get_name() << ": "
              << error_msg->parse().what() << std::endl;
    //assert(!"Failure");
    Glib::signal_idle().connect(sigc::mem_fun(this, &VideoProcessor::shutdown));
  }
  else if (msg->get_message_type() & Gst::MESSAGE_STATE_CHANGED)
  {
    Glib::RefPtr<Gst::MessageStateChanged> state_msg = Glib::RefPtr<Gst::MessageStateChanged>::cast_dynamic(msg);
    Gst::State oldstate;
    Gst::State newstate;
    Gst::State pending;
    state_msg->parse(oldstate, newstate, pending);

    std::cout << "message: " << msg->get_source()->get_name() << " old:" << to_string(oldstate) << " new:" << to_string(newstate) << std::endl;

    if (msg->get_source() == m_fakesink)
    {
      if (newstate == Gst::STATE_PAUSED)
      {
        std::cout << "                       --------->>>>>>> PAUSE" << std::endl;
      }

      if (newstate == Gst::STATE_PAUSED)
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
  else if (msg->get_message_type() & Gst::MESSAGE_EOS)
  {
    std::cout << "end of stream" << std::endl;
    Glib::signal_idle().connect(sigc::mem_fun(this, &VideoProcessor::shutdown));
  }
  else if (msg->get_message_type() & Gst::MESSAGE_TAG)
  {
    std::cout << "MESSAGE_TAG" << std::endl;
    Glib::RefPtr<Gst::MessageTag> tag_msg = Glib::RefPtr<Gst::MessageTag>::cast_dynamic(msg);
    Gst::TagList tag_list = tag_msg->parse();
    tag_list.foreach([](const Glib::ustring& tag){
        std::cout << "  tag: " << tag << std::endl;
      });
  }
  else if (msg->get_message_type() & Gst::MESSAGE_ASYNC_DONE)
  {
    std::cout << "MESSAGE_ASYNC_DONE" << std::endl;
  }
  else if (msg->get_message_type() & Gst::MESSAGE_STREAM_STATUS)
  {
    std::cout << "MESSAGE_STREAM_STATUS" << std::endl;
    Glib::RefPtr<Gst::MessageStreamStatus> status_msg = Glib::RefPtr<Gst::MessageStreamStatus>::cast_dynamic(msg);
    Gst::StreamStatusType type = status_msg->parse();
    std::cout << "StreamStatus:";
    switch(type)
    {
      case Gst::STREAM_STATUS_TYPE_CREATE:
        std::cout << "CREATE";
        break;

      case Gst::STREAM_STATUS_TYPE_ENTER:
        std::cout << "ENTER";
        break;

      case Gst::STREAM_STATUS_TYPE_LEAVE:
        std::cout << "LEAVE";
        break;

      case Gst::STREAM_STATUS_TYPE_DESTROY:
        std::cout << "DESTROY";
        break;

      case Gst::STREAM_STATUS_TYPE_START:
        std::cout << "START";
        break;

      case Gst::STREAM_STATUS_TYPE_PAUSE:
        std::cout << "PAUSE";
        break;

      case Gst::STREAM_STATUS_TYPE_STOP:
        std::cout << "PAUSE";
        break;

      default:
        std::cout << "???";
        break;
    }
    std::cout << std::endl;
  }
  else if (msg->get_message_type() & Gst::MESSAGE_REQUEST_STATE)
  {
    std::cout << "MESSAGE_REQUEST_STATE" << std::endl;
  }
  else if (msg->get_message_type() & Gst::MESSAGE_STEP_START)
  {
    std::cout << "MESSAGE_STEP_START" << std::endl;
  }
  else if (msg->get_message_type() & Gst::MESSAGE_REQUEST_STATE)
  {
    std::cout << "MESSAGE_REQUEST_STATE" << std::endl;
  }
  else if (msg->get_message_type() & Gst::MESSAGE_QOS)
  {
    std::cout << "MESSAGE_QOS" << std::endl;
  }
  else if (msg->get_message_type() & Gst::MESSAGE_LATENCY)
  {
    std::cout << "MESSAGE_LATENCY" << std::endl;
  }
  else if (msg->get_message_type() & Gst::MESSAGE_DURATION)
  {
    std::cout << "MESSAGE_DURATION" << std::endl;
  }
  else if (msg->get_message_type() & GST_MESSAGE_APPLICATION)
  {
    std::cout << "MESSAGE_APPLICATION" << std::endl;
  }
  else if (msg->get_message_type() & GST_MESSAGE_ELEMENT)
  {
    std::cout << "MESSAGE_ELEMENT" << std::endl;
  }
  else
  {
    std::cout << "unknown message: " << msg->get_message_type() << std::endl;
    Glib::signal_idle().connect(sigc::mem_fun(this, &VideoProcessor::shutdown));
  }
}

bool
VideoProcessor::shutdown()
{
  {
    Gst::State state;
    Gst::State pending;
    Gst::StateChangeReturn ret = m_playbin->get_state(state, pending, 0);
    std::cout << "--STATE: " << ret << " " << to_string(state) << " " << to_string(pending) << std::endl;
  }

  std::cout << "Going to shutdown!!!!!!!!!!!" << std::endl;
  m_playbin->set_state(Gst::STATE_NULL);
  m_mainloop->quit();
  return false;
}

bool
VideoProcessor::on_timeout()
{
  Glib::TimeVal t;
  t.assign_current_time();
  t = t - m_last_screenshot;

  std::cout << "TIMEOUT: " << t.as_double() << " " << m_timeout << std::endl;
  if (t.as_double() > m_timeout/1000.0)
  {
    std::cout << "--------- timeout ----------------: "  << t.as_double() << std::endl;
    Glib::signal_idle().connect(sigc::mem_fun(this, &VideoProcessor::shutdown));
  }
  return true;
}

/* EOF */
